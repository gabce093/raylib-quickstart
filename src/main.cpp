#include "raylib.h"
#include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include<stdio.h> 
#include <vector>
#include <array>
#include <iostream>
#include <cstdlib>
#include <string>   
#include <optional>


const struct SizeConfig {
	SizeConfig(int screenW, int screenH,size_t rows, size_t cols, float tileSize, float tilePadd, float boardPadd)
		: screenWidth{ screenW }, screenHeight{ screenH }, rows{ rows }, cols{cols},
		tileSize {tileSize}, tilePadding{ tilePadd }, boardPadding{ boardPadd } {};

	int const screenWidth;
	int const screenHeight;
	size_t rows;
	size_t cols;
	float const tileSize;
	float const tilePadding;
	float const boardPadding;
	float boardWidth = ((tileSize + tilePadding) * cols) - tilePadding;
	float boardHeight = ((tileSize + tilePadding) * rows) - tilePadding;
};

namespace Minesweeper {
	enum class TileState { Closed, Open, Flagged, Bomb, HeldDown};
	enum class GameState { Ongoing, Won, Lost };

    class Tile;

	template<size_t rows, size_t cols>
	class Solver;

	template<size_t rows, size_t cols>
	class InputHandler;

    template<typename T, size_t rows, size_t columns>
    class Matrix {
    public:
        using Row = std::array<T, columns>;
        using Column = std::array<T, rows>;

        explicit Matrix() : content{} { };

        friend std::ostream& operator<<(std::ostream& os, Matrix const& m) {

            for (auto& row : m.content) {
                os << "[ ";
                for (auto& element : row) {
                    os << element << ",";
                }
                os << " ]\n";
            }
            os << std::endl;
            return os;
        }

        Row& operator[](size_t row) {
            return content[row];
        }

        const Row& operator[](size_t row) const {
            return content[row];
        }
    protected:

        std::array<std::array<T, columns>, rows> content;
    };

	template<size_t rows, size_t cols>
	class Board {
	public:
		Board() : tiles{}, bombs{} {
			placeBombs();
			placeHints();
		}

		void placeBombs() {
			for (size_t i = 0; i < rows; i++)
			{
				for (size_t j = 0; j < cols; j++)
				{
					if (rand() % 4 == 0) {
						
						tiles[i][j] = Tile{ 0, true, TileState::Closed };
						bombs++;
					}
					else {
						tiles[i][j] = Tile{ };
					}
				}
			}
		}
		void placeHints() {
			std::vector<std::pair<int, int>>
				offsets{ {-1,-1},{-1, 0},{-1,1},
						 { 0,-1}        ,{ 0,1},
						 { 1,-1},{ 1, 0},{ 1,1} };
			for (size_t i = 0; i < rows; i++)
			{
				for (size_t j = 0; j < cols; j++)
				{

					if (tiles[i][j].isBomb()) {
						continue;
					}
					int bombCount = 0;
					for (auto& [dx, dy] : offsets) {
						size_t newRow = i + dx;
						size_t newCol = j + dy;
						if (newRow < rows && newCol < cols) {
							if (tiles[newRow][newCol].isBomb()) {
								bombCount++;
							}
						}
						tiles[i][j] = Tile{ bombCount };
					}
				}
			}
		}

		size_t getBombs() const {
			return bombs;
		}

		typename Matrix<Tile, rows, cols>::Row& operator[](size_t row) {
			return tiles[row];
		}

		const typename Matrix<Tile, rows, cols>::Row& operator[](size_t row) const {
			return tiles[row];
		}

		Board& operator--() {
			--bombs;
			return *this;
		}

		Board& operator++() {
			++bombs;
			return *this;
		}

		friend std::ostream& operator<<(std::ostream& os, Board const& b) {
			os << b.tiles;
			return os;
		}

	protected:
		Matrix<Tile, rows, cols> tiles;
		size_t bombs;

	};

	class Tile {
	public:
		Tile(int val = 0, bool isBomb = false, TileState state = TileState::Closed)
			:value{ val }, bomb{ isBomb }, state{ state }, heldDown{ false } {};

		TileState getState() const {return state;}

		bool isBomb() const { return bomb; }
		void setState(TileState st) { state = st; }
		void setHeldDown(bool state) { heldDown = state; }
		bool isHeldDown() const { return heldDown; }
		int getValue() const { return value; }

		friend std::ostream& operator<<(std::ostream& os, Tile const& t) {
			os << t.value;
			return os;
		}

	private:
		int value;
		bool bomb;
		bool heldDown;
		TileState state;
	};

	template<size_t rows, size_t cols>
	class Game {
	public:
		Game(SizeConfig const& conf) : state{ GameState::Ongoing }, sizeConfig{ conf }, startTime{ GetTime() }, endTime{} { initTiles(); initCheatButton(); }

		void toggleFlag(size_t row, size_t col) {
			if (getTile(row, col).getState() == TileState::Flagged) {
				board[row][col].setState(TileState::Closed);
				++board;
				return;
			}
			if (getTile(row, col).getState() == TileState::Closed) {
				board[row][col].setState(TileState::Flagged);
				--board;
			}
				
		}
		void toggleHeldDown(size_t row, size_t col, bool state) {
			board[row][col].setHeldDown(state);
		}

		void initTiles() {	
			float boardWidth = ((sizeConfig.tileSize + sizeConfig.tilePadding) * cols) - sizeConfig.tilePadding;
			float boardHeight = ((sizeConfig.tileSize + sizeConfig.tilePadding) * rows) - sizeConfig.tilePadding;

			float centerX = (sizeConfig.screenWidth - boardWidth) / 2;
			float centerY = (sizeConfig.screenHeight - boardHeight) / 2;

			for (int i = 0; i < rows * cols; i++)
			{
				int row = i / cols;
				int col = i % cols;

				tiles[i].x = centerX + col * (sizeConfig.tileSize + sizeConfig.tilePadding);
				tiles[i].y = centerY + row * (sizeConfig.tileSize + sizeConfig.tilePadding);
				tiles[i].width = sizeConfig.tileSize;
				tiles[i].height = sizeConfig.tileSize;
			}
		}
		void initCheatButton() {
			cheatButton = { 0.0f, 0.0f, 100.0f, 100.0f };
		}

		bool checkWin()  {
			for (size_t i = 0; i < rows; i++){
				for (Tile const& t : board[i]) {
					if (t.getState() != TileState::Open && !t.isBomb())
						return false;
				}
			}
			endTime = GetTime();
			return true;
		}
		void openTile(size_t row, size_t col) {
			
			board[row][col].setState(TileState::Open);

			if (board[row][col].isBomb()) {
				state = GameState::Lost;
				return;
			}
			if(checkWin())
				state = GameState::Won;
			revealTiles(row, col);
		}

		Rectangle getTileRect(size_t index) const { return tiles[index]; }
		const Tile& getTile(size_t row, size_t col) const {
			return board[row][col];
		}
		GameState getGameState() const { return state; }
		TileState getTileRenderState(size_t row, size_t col) const {
			const Tile& tile = board[row][col];

			if (tile.getState() == TileState::Open) {
				if (tile.isBomb()) {
					return TileState::Bomb;
				}
				return TileState::Open;
			}
			if (tile.isHeldDown())
				return TileState::HeldDown;
			return tile.getState();
		}
		int getBombs() const { return board.getBombs(); }
		int getGameTime() const { return endTime - startTime; }
		Rectangle getCheatButton() {
			return cheatButton;
		}

		void revealTiles(size_t row, size_t col) {
			if (board[row][col].getValue() != 0 || board[row][col].isBomb())
				return;

			std::vector<std::pair<int, int>>
				offsets{ {-1,-1},{-1, 0},{-1,1},
						 { 0,-1}        ,{ 0,1},
						 { 1,-1},{ 1, 0},{ 1,1} };
			loopAdjacentTiles(row, col, [&](size_t newRow, size_t newCol) {
				if (board[newRow][newCol].getValue() == 0 && board[newRow][newCol].getState() == TileState::Closed) {
					board[newRow][newCol].setState(TileState::Open);
					revealTiles(newRow, newCol);
				}
				board[newRow][newCol].setState(TileState::Open);
				});
		}
		void fastOpen(size_t row, size_t col) {
			int flags = 0;
			loopAdjacentTiles(row, col,[this, &flags](size_t newRow, size_t newCol) {
				if (board[newRow][newCol].getState() == TileState::Flagged)
					flags++;
			});
		
			if (flags == board[row][col].getValue()) {
				loopAdjacentTiles(row, col, [this, &flags](size_t newRow, size_t newCol) {
					if (board[newRow][newCol].getState() == TileState::Closed)
						openTile(newRow, newCol);
					});
			}
		}

		template<typename Call>
		void loopAdjacentTiles(size_t row, size_t col, Call callOnTiles) {
			std::vector<std::pair<int, int>>
				offsets{ {-1,-1},{-1, 0},{-1,1},
						 { 0,-1}        ,{ 0,1},
						 { 1,-1},{ 1, 0},{ 1,1} };

			size_t count = 0;
			for (auto& [dx, dy] : offsets) {
				size_t newRow = row + dx;
				size_t newCol = col + dy;
				if (newRow < rows && newCol < cols) {
					callOnTiles(newRow, newCol);
				}
			}
		}
		void hoverAdjacent(size_t row, size_t col, bool state) {
			loopAdjacentTiles(row, col, [&](size_t newRow, size_t newCol){
				if (board[newRow][newCol].getState() == TileState::Closed) {
					board[newRow][newCol].setHeldDown(state);
					}
				});
		}

	private:

		double startTime, endTime;
		Board<rows, cols> board;
		GameState state;
		Rectangle tiles[rows * cols];
		SizeConfig const& sizeConfig;
		Rectangle cheatButton;
	

	};

	template<size_t rows, size_t cols>
	class InputHandler {
	public:
		InputHandler() : mousePoint{} {};
		
		void handleInput(Game<rows, cols>& game) {
			mousePoint = GetMousePosition();
			if (CheckCollisionPointRec(mousePoint, game.getCheatButton())) {
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					Solver solver{ game };
					solver.solve();
				}
			}
			for (int i = 0; i < rows*cols; i++)
			{
				int row = i / cols;
				int col = i % cols;
				const Tile& currentTile = game.getTile(row, col);
				if (CheckCollisionPointRec(mousePoint, game.getTileRect(i)))
				{
					//Hover effect when holding down mouse
					if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && currentTile.getState() != TileState::Flagged ) {
						
						if (currentTile.getState() == TileState::Open) {
							game.hoverAdjacent(row, col, true);
							return;
						}
						else if (currentTile.getState() == TileState::Closed) {
							game.toggleHeldDown(row, col, true);
						}
					
					}

					if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
						//Open by clicking on an already open tile
						if(currentTile.getState() == TileState::Open) {
							game.toggleHeldDown(row, col, false); 
							game.hoverAdjacent(row, col, false);
							game.fastOpen(row, col);
						}
						//Open tile by clikcing on closed tile
						if (currentTile.getState() != TileState::Flagged && game.getGameState() == GameState::Ongoing) {
							//game.toggleHeldDown(row, col, false); 
							game.hoverAdjacent(row, col, false);
							game.openTile(row, col);
						}
					}
					//Put down a flag
					if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && game.getGameState() == GameState::Ongoing)
						game.toggleFlag(row, col);
				}
				else {
					game.toggleHeldDown(row, col, false);
				}
				
			}
		}
	private:
		Vector2 mousePoint;
		
	};

	template<size_t rows, size_t cols>
	class GameRenderer {
	public:
		GameRenderer(SizeConfig& s, Game<rows, cols> const& game) :
			sizeConfig{ s }, game{game} {
			Image bombPath = LoadImage("resources/bomb_1_ps.png");
			ImageFormat(&bombPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			bombTex = LoadTextureFromImage(bombPath);
			if (bombTex.id == 0) {
				std::cerr << "Failed to load bomb texture!" << std::endl;
			}

			Image flagPath = LoadImage("resources/red_flag_ps.png");
			ImageFormat(&flagPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			flagTex = LoadTextureFromImage(flagPath);
			if (flagTex.id == 0) {
				std::cerr << "Failed to load flag texture!" << std::endl;
			}
		};
		~GameRenderer() {
			UnloadTexture(bombTex);
			UnloadTexture(flagTex);
		}

		void drawGame() const {
			drawBombCounter();
			drawGameBoard();
			drawGameOverMessage();
			drawButton(0, 0, TextFormat("Press"));
		}
	
	private:
		void drawGameBoard() const {
			float centerX = (sizeConfig.screenWidth - sizeConfig.boardWidth) / 2;
			float centerY = (sizeConfig.screenHeight - sizeConfig.boardHeight) / 2;

			Rectangle gameBoard{ centerX - sizeConfig.boardPadding / 2,centerY - sizeConfig.boardPadding / 2,
								sizeConfig.boardWidth + sizeConfig.boardPadding, sizeConfig.boardHeight + sizeConfig.boardPadding };
			DrawRectangleRec(gameBoard, GRAY);

			for (int i = 0; i < cols * rows; ++i) {
				int row = i / cols;
				int col = i % cols;

				TileState state = game.getTileRenderState(row, col);
				Rectangle tileRect = game.getTileRect(i);

				switch (state) {
				case TileState::Open: {
					int tileValue = game.getTile(row, col).getValue();
					if (tileValue == 0) {
						DrawRectangleRounded(tileRect, 0.1f, 0, Fade(LIGHTGRAY, 0.6f));
						break;
					}

					DrawRectangleRounded(tileRect, 0.1f, 0, Fade(LIGHTGRAY, 0.6f));
					DrawText(TextFormat("%i", game.getTile(row, col).getValue()),
						tileRect.x + (sizeConfig.tileSize / 3), tileRect.y + (sizeConfig.tileSize / 4), 20, getNumberColor(tileValue));
					break;
				}
				case TileState::Bomb:
					DrawTexture(bombTex, tileRect.x, tileRect.y, WHITE);
					break;
				case TileState::Flagged:
					DrawRectangleRec(tileRect, DARKGRAY);
					DrawTexture(flagTex, tileRect.x, tileRect.y, WHITE);
					break;
				case TileState::HeldDown:
					DrawRectangleRounded(tileRect, 0.1f, 0, Fade(LIGHTGRAY, 0.6f));
					break;
				default:
					DrawRectangleRounded(tileRect, 0.1f, 0, DARKGRAY);

					break;
				}
			}
		}
		void drawBombCounter()const {
			float centerX = (sizeConfig.screenWidth - sizeConfig.boardWidth) / 2;
			float centerY = (sizeConfig.screenHeight - sizeConfig.boardHeight) / 2;
			int counterWidth = sizeConfig.screenWidth * 0.125;
			int counterHeight = sizeConfig.screenHeight * 0.075;
			Vector2 innerPad{ 0.1 * counterWidth, 0.2 * counterHeight };
			Font fontDefault{ 0 };
			Vector2 counterTextPos = { centerX + innerPad.x / 2 , centerY - counterHeight + innerPad.y / 2 };

			Vector2 measure = MeasureTextEx(Font{}, TextFormat("%i", game.getBombs()), 25.0f, 5.0f);

			Rectangle counter{ centerX - sizeConfig.boardPadding / 2,(centerY - sizeConfig.boardPadding / 2) - counterHeight,
								counterWidth, counterHeight };
			Rectangle counterInner{ (centerX - sizeConfig.boardPadding / 2) + innerPad.x / 2,(centerY - sizeConfig.boardPadding / 2) - counterHeight + innerPad.y / 2,
								counterWidth - innerPad.x, counterHeight - innerPad.y };

			DrawRectangleRec(counter, DARKGRAY);
			DrawRectangleRounded(counterInner, 0.1f, 0, { 140, 140, 140, 255 });
			DrawTextEx(fontDefault, TextFormat("%i", game.getBombs()), counterTextPos, 25, 5, RED);

		};
		void drawGameOverMessage()const {
			if (game.getGameState() == GameState::Ongoing)
				return;

			Vector2 fontPosition = { 0, sizeConfig.screenHeight / 2.0f - GetFontDefault().baseSize / 2.0f - 80.0f };
			if (game.getGameState() == GameState::Won) {
				const char winMsg[10] = "YOU WIN!";
				const char* timeMsg = TextFormat("Time: %ds", game.getGameTime());
				fontPosition.x = (sizeConfig.screenWidth - MeasureText(winMsg, 50)) / 2.0f;
				int center = (MeasureTextEx(GetFontDefault(), winMsg, 50.0f, 5.0f).x - MeasureTextEx(GetFontDefault(), timeMsg, 15.0f, 5.0f).x) / 2;

				Vector2 fontPositionTime = { (sizeConfig.screenWidth - MeasureText(winMsg, 50)) / 2.0f + center,
						  sizeConfig.screenHeight / 2.0f - GetFontDefault().baseSize / 2.0f - 80.0f + 50 };
				Rectangle msgBackground{ fontPosition.x, fontPosition.y, MeasureTextEx(GetFontDefault(),
					winMsg, 50.0f,5.0f).x, MeasureTextEx(GetFontDefault(), winMsg, 50.0f, 5.0f).y };
				Rectangle timeBackground{ fontPositionTime.x, fontPositionTime.y, MeasureTextEx(GetFontDefault(), timeMsg , 15.0f,5.0f).x, MeasureTextEx(GetFontDefault(), timeMsg, 15.0f, 5.0f).y };

				//Draw message
				DrawRectangleRounded(msgBackground, 0.1f, 0, DARKGRAY);
				DrawRectangleRounded(timeBackground, 0.1f, 0, DARKGRAY);
				DrawTextEx(GetFontDefault(), winMsg, fontPosition, 50.0f, 5, BLACK);
				DrawTextEx(GetFontDefault(), timeMsg, fontPositionTime, 15, 5, BLACK);
			}
			else if (game.getGameState() == GameState::Lost) {
				const char loseMsg[10] = "YOU LOSE!";
				fontPosition.x = (sizeConfig.screenWidth - MeasureText(loseMsg, 50)) / 2.0f;
				Rectangle msgBackground{ fontPosition.x, fontPosition.y, MeasureTextEx(GetFontDefault(), loseMsg, 50.0f,5.0f).x, MeasureTextEx(GetFontDefault(), loseMsg, 50.0f, 5.0f).y };

				//Draw message
				DrawRectangleRounded(msgBackground, 0.1f, 0, DARKGRAY);
				DrawTextEx(GetFontDefault(), loseMsg, fontPosition, 50.0f, 5, BLACK);
			}
		}
		void drawButton(float x, float y, const char* text) const {
			
			Rectangle button{x,y, 100.0f, 100.0f };
		
			DrawRectangleRounded(button, 0.1f, 0, DARKGRAY);
			DrawText(text, (int)x, (int)y, 15, BLACK);
		}

		Color getNumberColor(int tileValue)const{
			switch (tileValue) {
			case 1:
				return  {0,0,255,255};
				break;
			case 2:
				return {0, 119, 0, 255};
				break;
			case 3:
				return RED;
				break;
			case 4:
				return { 0,0,128,255 };
				break;
			case 5:
				return { 128, 0, 0, 255 };
				break;
			case 6:
				return {0, 128, 170, 255};
				break;
			default:
				return BLACK;
				break;
			}
		}
		
		Game<rows, cols> const& game;
		SizeConfig& sizeConfig;
		Texture2D bombTex;
		Texture2D flagTex;

	};

	template<size_t rows, size_t cols>
	class Solver {
	public:
		
		Solver(Game<rows, cols>& gameState) : gameState{ gameState } {};

		void solve() {
			for (size_t row = 0; row < rows; ++row) {
				for (size_t col = 0; col < cols; ++col) {
					const auto& tile = gameState.getTile(row, col);

					if (tile.getState() == TileState::Open && tile.getValue() != 0) {
						size_t flaggedTiles = countTileFlagged(row, col);
						size_t closedTiles = countTileClosed(row, col);
					
						if (closedTiles == tile.getValue()) {
							gameState.loopAdjacentTiles(row, col, [&](size_t newRow, size_t newCol) {
								if(gameState.getTile(newRow, newCol).getState() != TileState::Flagged)
									gameState.toggleFlag(newRow, newCol);
								});
							
						}
						if (flaggedTiles == tile.getValue()) {
							gameState.loopAdjacentTiles(row, col, [&](size_t newRow, size_t newCol) {
								if(gameState.getTile(newRow, newCol).getState() == TileState::Closed)
									gameState.openTile(newRow, newCol);
								});
						
						}
					}
					
				}
			}
		}

		size_t countTileClosed(size_t row, size_t col) {
			size_t count = 0;
			gameState.loopAdjacentTiles(row, col, [&](size_t newRow, size_t newCol)
				{
					if (gameState.getTile(newRow, newCol).getState() == TileState::Closed || gameState.getTile(newRow, newCol).getState() == TileState::Flagged)
						count++;
				});
			return count;
		}

		size_t countTileFlagged(size_t row, size_t col) {
			size_t count = 0;
			gameState.loopAdjacentTiles(row, col, [&](size_t newRow, size_t newCol)
				{
					if (gameState.getTile(newRow, newCol).getState() == TileState::Flagged)
						count++;
				});
			return count;
		}
		
	private:
		Game<rows, cols>& gameState;  
	};
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
	size_t const rows = 11;
	size_t const cols = 10;
	float const tileSize = 30.0f;
	float const tilePadding = 5.0f;
	float const boardPadding = 10.f;
	int const screenWidth= 800;
	int const screenHeight= 500;
	SizeConfig sizeConfig{ screenWidth, screenHeight,rows, cols, tileSize, tilePadding, boardPadding };

	InitWindow(sizeConfig.screenWidth, sizeConfig.screenHeight, "Minesweeper");
	SetTargetFPS(60);

	Minesweeper::Game<rows, cols> gameState{ sizeConfig };
	Minesweeper::GameRenderer<rows, cols> renderer{ sizeConfig, gameState };
	Minesweeper::InputHandler<rows, cols> inputHandler{};
	Minesweeper::Solver<rows, cols> solver{ gameState };

	// Main game loop
	while (!WindowShouldClose())
	{
		inputHandler.handleInput(gameState);

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();
		ClearBackground(RAYWHITE);

		renderer.drawGame();
	
		DrawFPS(700, 10);
		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

    return 0;
}
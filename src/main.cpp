

#include "raylib.h"
#include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include<stdio.h> 
#include <vector>
#include <array>
#include <iostream>
#include <cstdlib>
#include <string>   


namespace Minesweeper {
    class Tile;

	template<size_t rows, size_t cols>
	class InputHandler;

    template<typename T, size_t rows, size_t columns>
    class Matrix {
    public:
        using Row = std::array<T, columns>;
        using Column = std::array<T, rows>;

        explicit Matrix() : content{} { };

        void insert(size_t row, size_t column, T element) {
            content[row][column] = element;
        }

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
						
						tiles[i][j] = Tile{ 0, true };
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
		Tile(int val = 0, bool isBomb = false )
			:value{ val }, open{ false }, flagged{ false }, bomb{ isBomb } {}

		bool isBomb() const { return bomb; }
		bool isOpen() const { return open; }
		bool isFlagged() const { return flagged; }


		void setFlag(bool state) { flagged = state; }
		void openTile() { open = true; }

		int getValue() const { return value; }

		friend std::ostream& operator<<(std::ostream& os, Tile const& t) {
			os << t.value;
			return os;
		}

	private:
		int value;
		bool open;
		bool flagged;
		bool bomb;

	};

	template<size_t rows, size_t cols>
	class Game {
	public:
		Game() : isLive{ true }, board{} { play(); };

		void play() {
			const int screenWidth = 800;
			const int screenHeight = 450;
			InitWindow(screenWidth, screenHeight, "Minesweeper");
			SetTargetFPS(60);

			Image bombPath = LoadImage("resources/bomb_1_ps.png");   
			ImageFormat(&bombPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			Texture2D bombTex = LoadTextureFromImage(bombPath);

			Image flagPath = LoadImage("resources/red_flag_ps.png");
			ImageFormat(&flagPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			Texture2D flagTex = LoadTextureFromImage(flagPath);

			size_t const boardDim = rows * cols;
			float tileSize = 30.0f;
			float tilePadding = 5.0f;
			float boardPadding = 10.f;

			float boardWidth = ((tileSize + tilePadding) * cols)-tilePadding;
			float boardHeight = ((tileSize + tilePadding) * rows)-tilePadding;

			float centerX = (screenWidth - boardWidth) / 2;
			float centerY = (screenHeight - boardHeight) / 2;


			Rectangle tiles[boardDim] = { 0 };
			Rectangle gameBoard{ centerX - boardPadding / 2,centerY - boardPadding / 2, boardWidth + boardPadding,boardHeight + boardPadding };
			for (int i = 0; i < boardDim; i++)
			{
				int row = i / cols;
				int col = i % cols;
			
				tiles[i].x = centerX + col * (tileSize + tilePadding);
				tiles[i].y = centerY + row * (tileSize + tilePadding);
				tiles[i].width = tileSize;
				tiles[i].height = tileSize;
			}

			Vector2 mousePoint = { 0.0f, 0.0f };

			//InputHandler<rows, cols> inputHandler{};
			bool bombClicked = false;

			Font fontDefault{ 0 };
			const char winMsg[10] = "YOU WIN!";
			const char loseMsg[10] = "YOU LOSE!";
			Vector2 fontPositionWin = { (screenWidth - MeasureText(winMsg, 50))/ 2.0f,
							  screenHeight / 2.0f - fontDefault.baseSize / 2.0f - 80.0f };

			Vector2 fontPositionLose = { (screenWidth  - MeasureText(loseMsg, 50)) / 2,
							  screenHeight / 2.0f - fontDefault.baseSize / 2.0f - 80.0f };

			Vector2 measure = MeasureTextEx(Font{}, TextFormat("YOU WIN"), 50.0f, 0.0f);
			// Main game loop
			while (!WindowShouldClose())    
			{
				mousePoint = GetMousePosition();
				for (int i = 0; i < boardDim; i++)
				{
					int row = i / cols;
					int col = i % cols;
					if (CheckCollisionPointRec(mousePoint, tiles[i]))
					{
						
						if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !board[row][col].isFlagged() && isLive){
							bombClicked = checkTile(board[row][col]);
							revealTiles(row, col);
						}
						if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
							board[row][col].setFlag(!board[row][col].isFlagged());
					}
				}
				BombCounter b{Rectangle(0,0, 100, 100), board};
				// Draw
				//----------------------------------------------------------------------------------
				BeginDrawing();
				ClearBackground(RAYWHITE);
				b.display();
				
				DrawRectangleRec(gameBoard, GRAY);
				for (int i = 0; i < boardDim; ++i) {
					int row = i / cols;
					int col = i % cols;
					Tile currentTile = board[row][col];
					if (currentTile.isOpen()) {
						if (currentTile.isBomb()) {
							DrawTexture(bombTex, tiles[i].x, tiles[i].y, WHITE);
						}
						else
							DrawText(TextFormat("%i", board[row][col].getValue()), tiles[i].x+(tileSize/3), tiles[i].y+(tileSize / 4), 20, BLUE);
					}
					else {
						DrawRectangleRec(tiles[i], DARKGRAY);
						if (currentTile.isFlagged()) 
							DrawTexture(flagTex, tiles[i].x, tiles[i].y, WHITE);
					}
				}
				if (bombClicked) 
					DrawTextEx(fontDefault, loseMsg, fontPositionLose, 50.0f, 5, BLACK);
				
				if(checkWin())
					DrawTextEx(fontDefault, winMsg, fontPositionWin, 50.0f, 5, BLACK);

				DrawFPS(10, 10);

				EndDrawing();
				//----------------------------------------------------------------------------------
			}

			// De-Initialization
			//--------------------------------------------------------------------------------------
			CloseWindow();        // Close window and OpenGL context
			//--------------------------------------------------------------------------------------
		
		}

		

		void revealTiles(size_t row, size_t col) {

			if (!board[row][col].getValue() == 0 || board[row][col].isBomb())
				return;

			std::vector<std::pair<int, int>>
				offsets{ {-1,-1},{-1, 0},{-1,1},
						 { 0,-1}        ,{ 0,1},
						 { 1,-1},{ 1, 0},{ 1,1} };

			for (auto& [dx, dy] : offsets) {
				size_t newRow = row + dx;
				size_t newCol = col + dy;
				if (newRow < rows && newCol < cols) {

					if (board[newRow][newCol].getValue() == 0 && !board[newRow][newCol].isOpen()) {
						revealTiles(newRow, newCol);
					}
					board[newRow][newCol].openTile();
				}

			}
		}

		bool checkWin() {
			for (size_t i = 0; i < rows; i++){
				for (Tile& t : board[i]) {
					if (!t.isOpen() && !t.isBomb())
						return false;
				}
			}
			isLive = false;
			return true;
		}

		bool checkTile(Tile& tile) {
			tile.openTile();

			if (tile.isBomb())
				return true;
			return false;
		}

		friend std::ostream& operator<<(std::ostream& os, Game const& g) {
			os << "Bombs Left: " << g.board.getBombs() << "\n";
			for (size_t i = 0; i < rows; i++)
			{
				os << "[";
				for (size_t j = 0; j < cols; j++)
				{
					if (g.board[i][j].isOpen()) {
						if (g.board[i][j].isBomb()) {
							os << " " << "X";
						}
						else {
							os << " " << g.board[i][j];
						}

					}
					else if (g.board[i][j].isFlagged()) {
						os << " " << ">";
					}
					else {
						os << " O";
					}
				}
				os << " ]\n";
			}
			return os;
		}

	private:

		class BombCounter {
		public:
			BombCounter(Rectangle r, const Board<rows, cols>& b) : counter{ r }, boardRef{b} {};
			void display() {
				DrawRectangleRec(counter, GREEN);
				DrawText(TextFormat("%i", boardRef.getBombs()),counter.x, counter.y, 25, BLACK);
			}
		private:
			Rectangle counter;
			const Board<rows, cols>& boardRef;
		};

		bool isLive;
		Board<rows, cols> board;

	};

	//template<size_t rows, size_t cols>
	//class InputHandler {
	//public:

	//	InputHandler() : mousePoint{} {};

	//	void handleInput(Game& game) {
	//		mousePoint = GetMousePosition();

	//		// Loop through the tiles and detect mouse click
	//		for (size_t i = 0; i < rows * cols; i++) {
	//			size_t row = i / cols;
	//			size_t col = i % cols;

	//			if (CheckCollisionPointRec(mousePosition, tiles[i])) {
	//				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
	//					game.openTile(row, col);  // Left-click: open the tile
	//				}
	//				if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
	//					game.toggleFlag(row, col);  // Right-click: toggle flag
	//				}
	//			}
	//		}
	//	}
	//	void updateMousePosition() {};

	//private:
	//	Vector2 mousePoint;
	//};

}




//template<size_t rows, size_t cols>
//class GameRenderer {
//public:
//	void drawGameBoard(const Game<rows, cols>& game) {
//
//	}
//	void drawTile(const Tile& tile, float x, float y);
//	void drawBombCounter(int bombCount);
//	void drawGameOverMessage(GameState state);
//};
//
//class InputHandler {
//public:
//	void handleMouseInput(Game& game);
//	void updateMousePosition();
//
//private:
//	Vector2 mousePoint;
//};

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
	Minesweeper::Game<10, 10> g;
    return 0;
}
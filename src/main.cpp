#include "raylib.h"
#include "resource_dir.h" // utility header for SearchAndSetResourceDir

#include <stdio.h> 
#include <vector>
#include <array>
#include <iostream>
#include <cstdlib>
  
#include <utility> 
#include <climits>
#include <random>

#include "myMatrix.h"
#include "enums.h"
#include "tile.h"

#include "random_iterator.h"


struct SizeConfig {
	SizeConfig() {};
	int const screenWidth = 1700;
	int const screenHeight= 900;
	size_t rows = 0;
	size_t cols = 0;
	float const tileSize = 30.0f;
	float const tilePadding = 0.0f;
	float const boardPadding = 10.f;
	float boardWidth = ((tileSize + tilePadding) * cols) - tilePadding;
	float boardHeight = ((tileSize + tilePadding) * rows) - tilePadding;

	void update() {
		boardWidth = ((tileSize + tilePadding) * cols) - tilePadding;
		boardHeight = ((tileSize + tilePadding) * rows) - tilePadding;
	}
};


namespace Minesweeper {

	class Board {
	public:
		Board(size_t rows = 0, size_t cols = 0, Difficulty diff = Difficulty::Easy) : tiles{ rows, cols }, numOfBombs{} {
			placeBombs(diff);
			placeHints();
		}

		virtual void placeBombs(Difficulty diff) {
			numOfBombs =  (int)(tiles.size() * bombPercentage(diff));

			//POTENTIAL ERROR IF BOARD IS size() == 1
			RandomIterator iterator(numOfBombs, 0, tiles.size()-1);
			
			while (iterator.has_next()) {
				size_t tileIndex = iterator.next();
				size_t row = tileIndex / tiles.size(1);
				size_t col = tileIndex % tiles.size(1);
				tiles[row][col] = { 0, true, TileState::Closed };
			}
		
	
		}

		template<typename Call>
		void loopAdjTiles(size_t row, size_t col, Call callOnTiles) {
			std::vector<std::pair<int, int>>
				offsets{ {-1,-1},{-1, 0},{-1,1},
						 { 0,-1}        ,{ 0,1},
						 { 1,-1},{ 1, 0},{ 1,1} };

			for (auto& [dx, dy] : offsets) {
				size_t newRow = row + dx;
				size_t newCol = col + dy;
				if (newRow < tiles.size(0) && newCol < tiles.size(1)) {
					callOnTiles(newRow,newCol);
				}
			}
		}

		template<typename Call>
		void loopAdjTiles(size_t row, size_t col, Call callOnTiles) {
			std::vector<std::pair<int, int>>
				offsets{ {-1,-1},{-1, 0},{-1,1},
						 { 0,-1}        ,{ 0,1},
						 { 1,-1},{ 1, 0},{ 1,1} };

			for (auto& [dx, dy] : offsets) {
				size_t newRow = row + dx;
				size_t newCol = col + dy;
				if (newRow < tiles.size(0) && newCol < tiles.size(1)) {
					callOnTiles(tiles[newRow][newCol]);
				}
			}
		}
	


		virtual void placeHints() {
			std::vector<std::pair<int, int>>
				offsets{ {-1,-1},{-1, 0},{-1,1},
						 { 0,-1}        ,{ 0,1},
						 { 1,-1},{ 1, 0},{ 1,1} };
			for (size_t i = 0; i < tiles.size(0); i++)
			{
				for (size_t j = 0; j < tiles.size(1); j++)
				{
					if (tiles[i][j].isBomb()) {
						continue;
					}
					int bombCount = 0;
					for (auto& [dx, dy] : offsets) {
						size_t newRow = i + dx;
						size_t newCol = j + dy;
						if (newRow < tiles.size(0) && newCol < tiles.size(1)) {
							if (tiles[newRow][newCol].isBomb()) {
								bombCount++;
							}
						}
						tiles[i][j] = Tile{ bombCount };
					}
				}
			}
		}

		float bombPercentage(Difficulty difficulty) const {
			float bombPercentage = 0.f; 
			if (difficulty == Difficulty::Easy)
				bombPercentage = 0.1f; 
			else if (difficulty == Difficulty::Medium) 
				bombPercentage = 0.2f; 
			else if (difficulty == Difficulty::Hard) 
				bombPercentage = 0.3f;
			return bombPercentage;
		}

		int getBombs() const {
			return numOfBombs;
		}

		typename Matrix<Tile>::Row operator[](size_t row) {
			return tiles[row];
		}

		const typename Matrix<Tile>::Row operator[](size_t row) const {
			return tiles[row];
		}

	protected:
		Matrix<Tile> tiles;
		int numOfBombs;
	

	};
	enum class TileCondition { OpenAndFree, OpenOrMine, PossibleMine, PermanentMine };

	class NoGuessTile : public Tile {
	public:
		NoGuessTile(TileCondition cond) : Tile{}, condition{ cond } {};

	private:
		TileCondition condition;
	};
	struct Position {
		size_t row;
		size_t column;
	};

	class NoGuessBoard : public Board {
	public:
		NoGuessBoard() :startPos{} {};
		void placeBombs(Difficulty diff) override {
			RandomIterator iterator(numOfBombs, 0, tiles.size() - 1);
			size_t randPos = iterator.next();
			Position startPos = { randPos / tiles.size(1), randPos % tiles.size(1) };

			std::vector<Position> safe;
			safe.reserve(8);
			/*loopAdjTiles(startPos.row, startPos.column, [this](Tile t) { 
				t = NoGuessTile{ TileCondition::OpenAndFree };
				});*/

			while (iterator.has_next()) {
				size_t tileIndex = iterator.next();
				Position currentPos{ tileIndex / tiles.size(1), tileIndex % tiles.size(1) };
				
			
				tiles[currentPos.row][currentPos.column] = { 0, true, TileState::Closed };
			}

		};

		virtual void placeHints() override {

		}

	private:
		Position startPos;
	};



	class Button {
	public:
		Button() = default;
		Button(const char* text, Rectangle rec, GameScreen s, Color tc = DARKGRAY) : text{ text }, rect{ rec }, screen{ s }, heldDown{ false }, textColor {tc}{}

		const char* getText() const {
			return text;
		}

		const Rectangle getButtonRect() const {
			return rect;
		}

		const Color& getTextColor() const{
			return textColor;
		}

		const Vector2 getPosition() const {
			return { rect.x, rect.y };
		}

		GameScreen getScreen() const {
			return screen;
		}
		void setHeldDown(bool state) { heldDown = state; }
		bool isHeldDown() const { return heldDown; }

	private:
		Rectangle rect;
		GameScreen screen;
		const char* text;
		bool heldDown;
		Color textColor;
	};

	class Game {
	public:
		Game(SizeConfig& conf) : state{ GameState::Ongoing }, sizeConfig{ conf }, startTime{ GetTime() }, endTime{}, board{}, tiles{ }, bombCount{board.getBombs()} {  
			tryAgainButton = { (const char*)"Reset Game", { sizeConfig.screenWidth/2 -175.0f, 500.0f, 350.0f, 90.0f }, GameScreen::GAMEPLAY };
			continueButton = { (const char*)"Continue", { sizeConfig.screenWidth / 2 - 175.0f, 500.0f, 350.0f, 90.0f }, GameScreen::GAMEPLAY };
			homeButton = { (const char*)"Main Menu", { sizeConfig.screenWidth / 2 - 175.0f, 600.0f, 350.0f, 90.0f }, GameScreen::GAMEPLAY };
		}

		void startGame(size_t rows, size_t cols, Difficulty diff){
			sizeConfig.rows = rows;
			sizeConfig.cols = cols;
			sizeConfig.update();
			startTime = GetTime();
			
			board = Board{ rows, cols, diff };
			bombCount = board.getBombs();
			tiles = Matrix<Rectangle>{ rows, cols };
			initTiles();
		}
		void resetGame() {
			for (size_t i = 0; i < sizeConfig.rows; i++)
			{
				for (size_t j = 0; j < sizeConfig.cols; j++)
				{
					if (board[i][j].getState() != TileState::Closed)
						board[i][j].setState(TileState::Closed);
				}
			}
			startTime = GetTime();
			bombCount = board.getBombs();
			state = GameState::Ongoing;
		}
		void continueGame() {
			for (size_t i = 0; i < sizeConfig.rows; i++)
			{
				for (size_t j = 0; j < sizeConfig.cols; j++)
				{
					if (board[i][j].isBomb() && board[i][j].getState() != TileState::Flagged)
						board[i][j].setState(TileState::Closed); 
				}
			}
			state = GameState::Ongoing;
		}

		void toggleFlag(size_t row, size_t col) {
			if (getTile(row, col).getState() == TileState::Flagged) {
				board[row][col].setState(TileState::Closed);
				++bombCount;
				return;
			}
			if (getTile(row, col).getState() == TileState::Closed) {
				board[row][col].setState(TileState::Flagged);
				--bombCount;
			}
				
		}
		void toggleHeldDown(size_t row, size_t col, bool held) {
			board[row][col].setHeldDown(held);
		}
		void initTiles() {	
			float boardWidth = ((sizeConfig.tileSize + sizeConfig.tilePadding) * sizeConfig.cols) - sizeConfig.tilePadding;
			float boardHeight = ((sizeConfig.tileSize + sizeConfig.tilePadding) * sizeConfig.rows) - sizeConfig.tilePadding;

			float centerX = (sizeConfig.screenWidth - boardWidth) / 2;
			float centerY = (sizeConfig.screenHeight - boardHeight) / 2;
			
			for (size_t row = 0; row < sizeConfig.rows; row++)
			{
				for (size_t col = 0; col < sizeConfig.cols; col++)
				{
				tiles[row][col].x = centerX + col * (sizeConfig.tileSize + sizeConfig.tilePadding);
				tiles[row][col].y = centerY + row * (sizeConfig.tileSize + sizeConfig.tilePadding);
				tiles[row][col].width = sizeConfig.tileSize;
				tiles[row][col].height = sizeConfig.tileSize;
				}
			}
		}
		
		bool checkWin()  { 
			for (size_t i = 0; i < sizeConfig.rows; i++){
				for (size_t j = 0; j < sizeConfig.cols; j++)
				{
					Tile t = board[i][j];
					if (t.getState() != TileState::Open && !t.isBomb()) 
						return false;
				}
			}
			endTime = GetTime();
			return true;
		}

		Position findTile(Tile const& t) {
			for (size_t i = 0; i < sizeConfig.rows; i++) {
				for (size_t j = 0; j < sizeConfig.cols; j++){
					if (board[i][j] == t) {
						return { i,j };
					}
				}
			}
		}
		void openTile(Tile& tile) {
			
			tile.setState(TileState::Open); 

			if (tile.isBomb()) { 
				state = GameState::Lost;
				return;
			}
			if(checkWin())
				state = GameState::Won;
			Position p = findTile(tile);
			revealTiles(p.row, p.column);
		}

		Rectangle getTileRect(size_t row, size_t col) const { return tiles[row][col]; }
		Tile& getTile(size_t row, size_t col) const {
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
		size_t getBombs() const { return bombCount; }
		double getGameTime() const {
			if(state == GameState::Won || state == GameState::Lost)
				return endTime - startTime;
			return 55.0;
			//std::cerr << "Cant get GameTime because game is not over!";
		}
		const Button& getTryAgainButton() const {
			return tryAgainButton;
		}
		const Button& getHomeButton() const {
			return homeButton;
		}
		const Button& getContinueButton() const{
			return continueButton;
		}
		
		void revealTiles(size_t row, size_t col) {
			if (board[row][col].getValue() != 0 || board[row][col].isBomb())
				return;


			board.loopAdjTiles(row, col, [&](size_t newRow, size_t newCol) {
				if (board[newRow][newCol].getValue() == 0 && board[newRow][newCol].getState() == TileState::Closed) {
					board[newRow][newCol].setState(TileState::Open);
					revealTiles(newRow, newCol);
				}
				board[newRow][newCol].setState(TileState::Open);
				});
		}
		void fastOpen(size_t row, size_t col) {
			int flags = 0;
			board.loopAdjacentTiles(row, col,[this, &flags](size_t newRow, size_t newCol) {
				if (board[newRow][newCol].getState() == TileState::Flagged)
					flags++;
			});
		
			if (flags == board[row][col].getValue()) {
				board.loopAdjacentTiles(row, col, [this, &flags](size_t newRow, size_t newCol) { 
					if (board[newRow][newCol].getState() == TileState::Closed)
						openTile(newRow, newCol);
					});
			}
		}
		
		void hoverAdjacent(size_t row, size_t col, bool pushed) {
			board.loopAdjTiles(row, col, [&pushed](Tile& tile){
				if (tile.getState() == TileState::Closed) {
					tile.setHeldDown(pushed);
					}
				});
		}
		
	private:
		int bombCount;
		double startTime, endTime;
		Board board;
		GameState state;
		SizeConfig& sizeConfig;
		Matrix<Rectangle> tiles;
		Button tryAgainButton, homeButton, continueButton;
	};

	class Menu {
	public:

		Menu(SizeConfig const& conf) : buttons{}, sizeConfig{ conf } {
			float btnVertPadd = 30.0f;
			float menuBtnWidth = 350.0f;
			float menuBtnHeight = 90.0f;
			const char* menuText[MENU_BUTTONS] = { "Play", "Settings", "How to play" };
			GameScreen menuScreens[MENU_BUTTONS] = { GameScreen::GAMEPLAY , GameScreen::SETTINGS, GameScreen::HOW_TO };
			float menuHeight = (MENU_BUTTONS * menuBtnHeight) + (MENU_BUTTONS - 1) * btnVertPadd;

			for (int i = 0; i < MENU_BUTTONS; i++)
			{
				float x = (sizeConfig.screenWidth - menuBtnWidth) / 2;
				float y = (sizeConfig.screenHeight / 2) - (menuHeight / 2) + (i * (btnVertPadd + menuBtnHeight));
				buttons[i] = Button{ menuText[i], {x, y, menuBtnWidth, menuBtnHeight}, menuScreens[i]};
			}
		}

		Button& getButton(size_t index)  {
			return buttons[index];
		}

		Button getButton(size_t index) const {
			return buttons[index];
		}

		size_t size() const {
			return MENU_BUTTONS;
		}

	
	private:
		static const int MENU_BUTTONS = 3;
		std::array<Button, MENU_BUTTONS> buttons;
		
		SizeConfig const& sizeConfig;
	};

	class TextBox {
	public:
		TextBox() = default;
		TextBox(Rectangle rect) : rect{ rect }, mouseOnBox{ false }, letterCount{0} {}

		const Rectangle& getRect() const {
			return rect;
		}
		bool isOn() const {
			return mouseOnBox;
		}

		void setMouseOn(bool state) {
			mouseOnBox = state;
		}

		int getLetterCount() const {
			return letterCount;
		}

		TextBox& operator++() {
			letterCount++;
			return *this;
		}
		TextBox& operator--() {
			letterCount--;
			return *this;
		}

		void addInput(char c, int index) {
			if (index < MAX_INPUT_CHARS && index >= 0)
				inputDim[index] = c;
		}

		void setLetterCount(int count) {
			letterCount = count;
		}

		const char* getInput() {
			return inputDim;
		}

		int getInput_int() {
			
			int num1 = inputDim[0] - '0'; // 4
			if (letterCount == 1)
				return num1;
			int num2 = inputDim[1] - '0'; // 7
			

			return num1 * 10 + num2;
		}


		static const int MAX_INPUT_CHARS = 2;
	private:
		Rectangle rect;
		bool mouseOnBox;
		int letterCount;
	
		char inputDim[MAX_INPUT_CHARS + 1] = "\0";
	};

	class Settings {
	public:
		Settings(SizeConfig& conf) : sizeConfig{ conf }, difficulty{Difficulty::Easy} {
			const char* enterText = "Enter the size of the board:";
			int enterTextWidth = MeasureText(enterText, 20);

			Rectangle textBoxRow = { sizeConfig.screenWidth / 2.0f - (enterTextWidth / 2) , 180, 100, 50 };
			float padTextBox = enterTextWidth - textBoxRow.width * 2;
			Rectangle textBoxCol = { textBoxRow.x + padTextBox + textBoxRow.width, textBoxRow.y, textBoxRow.width, textBoxRow.height };
			rowsBox = { textBoxRow };
			colsBox = { textBoxCol };
			playButton = { (const char*)"Play!", {sizeConfig.screenWidth / 2.0f- 175.0f,400,350,90 }, GameScreen::GAMEPLAY };

			float buttonPosY = 250;
			easy = { (const char*)"Easy", {sizeConfig.screenWidth / 2.0f - 175.0f -360.0f,buttonPosY,350,90 }, GameScreen::SETTINGS,  {0, 119, 0, 255} };
			medium = { (const char*)"Medium", {sizeConfig.screenWidth / 2.0f - 175.0f,buttonPosY,350,90 }, GameScreen::SETTINGS, {0,0,255,255} };
			hard = { (const char*)"Hard", {sizeConfig.screenWidth / 2.0f - 175.0f+360.0f,buttonPosY,350,90 }, GameScreen::SETTINGS, RED };

		}

		TextBox& getDimBoxes(size_t dim) {
			if (dim == 0)
				return rowsBox;
			return colsBox;
		}

		const TextBox& getDimBoxes(size_t dim) const {
			if (dim == 0)
				return rowsBox;
			return colsBox;
		}
	
		bool isOnBox(TextBox const& box) const {
			return box.isOn();
		}

		const Button& getPlayButton() const{
			return playButton;
		}

		 Button& getDifficultyButton(Difficulty d) {
			if (d == Difficulty::Easy)
				return easy;
			if (d == Difficulty::Medium)
				return medium;
		
				return hard;
		}

		 const Button& getDifficultyButton(Difficulty d) const {
			 if (d == Difficulty::Easy)
				 return easy;
			 if (d == Difficulty::Medium)
				 return medium;
				 return hard;
		 }

		Difficulty getDifficulty()const {
			return difficulty;
		}

		void setDifficulty(Difficulty diff) {
			difficulty = diff;
		}

	
	private:
		SizeConfig& sizeConfig;
		TextBox rowsBox, colsBox;
		Button playButton;
		Button easy, medium, hard;
		Difficulty difficulty;
	};

	class InputHandler {
	public:
		InputHandler(SizeConfig& conf) : mousePoint{}, sizeConfig{conf} {};
		
		GameScreen handleGameInput(Game& game) {
			

			if (game.getGameState() != GameState::Ongoing && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				if (CheckCollisionPointRec(mousePoint, game.getTryAgainButton().getButtonRect()) && game.getGameState() == GameState::Won) {
					game.resetGame();
					return GameScreen::GAMEPLAY;
				}
				if (CheckCollisionPointRec(mousePoint, game.getContinueButton().getButtonRect()) && game.getGameState() == GameState::Lost) {
					game.continueGame();
					return GameScreen::GAMEPLAY;
				}
				if (CheckCollisionPointRec(mousePoint, game.getHomeButton().getButtonRect())) {
					game.resetGame();
					return GameScreen::TITLE;
				}
			}
			for (int row = 0; row < sizeConfig.rows ; row++)
			{
				for (size_t col = 0; col < sizeConfig.cols; col++)
				{
					Tile& currentTile = game.getTile(row, col);
					if (CheckCollisionPointRec(mousePoint, game.getTileRect(row, col)))
					{
						//Hover effect when holding down mouse
						if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && currentTile.getState() != TileState::Flagged ) {
						
							if (currentTile.getState() == TileState::Open) {
								game.hoverAdjacent(row, col, true);
								return GameScreen::GAMEPLAY;
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
			return GameScreen::GAMEPLAY;
		}
		GameScreen handleMenuInput(Menu& menu) {


			for (size_t i = 0; i < menu.size(); i++)
			{
				Button& currentButton = menu.getButton(i);
				if (CheckCollisionPointRec(mousePoint, currentButton.getButtonRect()) )
				{
					if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
						currentButton.setHeldDown(true);

					if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
						currentButton.setHeldDown(false);
						if (sizeConfig.rows == 0 && currentButton.getScreen() == GameScreen::GAMEPLAY)
							return GameScreen::SETTINGS;
						return currentButton.getScreen();
					}
				}
				else {
					currentButton.setHeldDown(false);
				}
			}
			return GameScreen::TITLE;
		}
		GameScreen handleSettingsInput(Settings& settings, Game& game) {
			
			if (CheckCollisionPointRec(mousePoint, settings.getPlayButton().getButtonRect())) {
		
				if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
					//If the user has entered the board dimensions
					if (settings.getDimBoxes(0).getLetterCount() != 0 && settings.getDimBoxes(1).getLetterCount() != 0)
						sizeConfig.rows = settings.getDimBoxes(0).getInput_int();
						sizeConfig.cols = settings.getDimBoxes(1).getInput_int();
						game.startGame(sizeConfig.rows, sizeConfig.cols, settings.getDifficulty());
						return GameScreen::GAMEPLAY;
				}
			}
			handleDifficultyBtn(settings, Difficulty::Easy);
			handleDifficultyBtn(settings, Difficulty::Medium);
			handleDifficultyBtn(settings, Difficulty::Hard);
			for (size_t i = 0; i < 2; i++)
			{
				TextBox& currentBox = settings.getDimBoxes(i);
				TextBox& otherBox = settings.getDimBoxes(std::abs((int)i-1));
			
				if (CheckCollisionPointRec(mousePoint, currentBox.getRect()) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					currentBox.setMouseOn(true);
					otherBox.setMouseOn(false);
				}

				if (settings.isOnBox(currentBox))
				{
					// Set the window's cursor to the I-Beam
					SetMouseCursor(MOUSE_CURSOR_IBEAM);

					// Get char pressed (unicode character) on the queue
					int key = GetCharPressed();

					// Check if more characters have been pressed on the same frame
					while (key > 0)
					{
						// NOTE: Only allow keys in range [48..57]
						if ((key >= 48) && (key <= 57) && (currentBox.getLetterCount() < 2))
						{
							
							if (!(currentBox.getLetterCount() == 0 && key == 48)){
								currentBox.addInput((char)key, currentBox.getLetterCount());
								currentBox.addInput('\0', currentBox.getLetterCount() + 1);
								++currentBox;
							}
							
						}

						key = GetCharPressed();  // Check next character in the queue
					}

					if (IsKeyPressed(KEY_BACKSPACE))
					{
						--currentBox;
						if (currentBox.getLetterCount() < 0) currentBox.setLetterCount(0);
						currentBox.addInput('\0', currentBox.getLetterCount());
					}
				}
				else SetMouseCursor(MOUSE_CURSOR_DEFAULT);
			}
			return GameScreen::SETTINGS;
		};

		void updateMousePosition() {
			mousePoint = GetMousePosition();
		}
	private:
		void handleDifficultyBtn(Settings& settings, Difficulty diff) {
			if (CheckCollisionPointRec(mousePoint, settings.getDifficultyButton(diff).getButtonRect()) ) {
				

				//if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) 
					//settings.getDifficultyButton(diff).setHeldDown(true);  

				if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
					//settings.getDifficultyButton(diff).setHeldDown(false);
					settings.setDifficulty(diff); 
				}
			}

			if (settings.getDifficulty() == diff) {
				settings.getDifficultyButton(diff).setHeldDown(true);
			}
			else {
				settings.getDifficultyButton(diff).setHeldDown(false);
			}
		}

		Vector2 mousePoint;
		SizeConfig& sizeConfig;
		
	};

	class Renderer {
	public:
		Renderer(SizeConfig const& s) :
			sizeConfig{ s }, framesCounter{} {
			Image bombPath = LoadImage("resources/bomb_1_ps.png");
			ImageFormat(&bombPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			bombTex = LoadTextureFromImage(bombPath);
			if (bombTex.id == 0) {
				std::cerr << "Failed to load bomb texture!" << std::endl;
			}

			Image flagPath = LoadImage("resources/red_flag_20.png");
			ImageFormat(&flagPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			flagTex = LoadTextureFromImage(flagPath);
			if (flagTex.id == 0) {
				std::cerr << "Failed to load flag texture!" << std::endl;
			}

			Image tileUpPath = LoadImage("resources/cellup.png");
			ImageFormat(&tileUpPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			tileUpTex = LoadTextureFromImage(tileUpPath);
			if (tileUpTex.id == 0) {
				std::cerr << "Failed to load tileUp texture!" << std::endl;
			}
			Image tileDownPath = LoadImage("resources/celldown.png");
			ImageFormat(&tileDownPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			tileDownTex = LoadTextureFromImage(tileDownPath);
			if (tileDownTex.id == 0) {
				std::cerr << "Failed to load tileUp texture!" << std::endl;
			}

			Image menuBtnPath = LoadImage("resources/menu_button.png");
			ImageFormat(&menuBtnPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			menuBtnTex = LoadTextureFromImage(menuBtnPath);
			if (menuBtnTex.id == 0) {
				std::cerr << "Failed to load tileUp texture!" << std::endl;
			}
			Image menuBtnDownPath = LoadImage("resources/menu_button_down.png");
			ImageFormat(&menuBtnDownPath, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			menuBtnDownTex = LoadTextureFromImage(menuBtnDownPath);
			if (menuBtnDownTex.id == 0) {
				std::cerr << "Failed to load tileUp texture!" << std::endl;
			}
		};
		~Renderer() {
			UnloadTexture(bombTex);
			UnloadTexture(flagTex);
			UnloadTexture(tileUpTex);
			UnloadTexture(tileDownTex);
		}

		void drawGame(Game const& game) const {
			
			drawGameBoard(game);
			drawBombCounter(game);
			drawGameOverMessage(game);

		}
		void drawMenu(Menu const& menu) {
			DrawText(TextFormat("MINESWEEPER"), (sizeConfig.screenWidth - MeasureText("MINESWEEPER", 75)) / 2, 100, 75, DARKGREEN);
			for (size_t i = 0; i < menu.size(); i++)
			{
				drawMenuButton(menu.getButton(i));
			}
		}
		void drawSettings(Settings const& settings) {
			const char* enterText = "Enter the size of the board:";
			int enterTextWidth = MeasureText(enterText, 20);

			Rectangle recSource{ 0,0, (float)menuBtnTex.width, (float)menuBtnTex.height };
			const TextBox& textBox = settings.getDimBoxes(0);

			float padTextBox = enterTextWidth - textBox.getRect().width * 2;
			DrawText(enterText, sizeConfig.screenWidth / 2- (enterTextWidth / 2), 140, 20, GRAY); 
			DrawText((const char*)"X", (int)(textBox.getRect().x + textBox.getRect().width + (padTextBox / 2) - MeasureText("X", 20) / 2), (int)textBox.getRect().y + 20, 20, GRAY);
			
			drawMenuButton(settings.getDifficultyButton(Difficulty::Easy));
			drawMenuButton(settings.getDifficultyButton(Difficulty::Medium));
			drawMenuButton(settings.getDifficultyButton(Difficulty::Hard));
			drawMenuButton(settings.getPlayButton());

			//DrawText(TextFormat("INPUT CHARS: %i/%i", letterCount, MAX_INPUT_CHARS),750 , 300, 20, DARKGRAY);
			TextBox box{};
			for (size_t i = 0; i < 2; i++)
			{
				if (i == 0)
					 box = settings.getDimBoxes(0);
				else
				     box = settings.getDimBoxes(1);

				DrawRectangleRec(box.getRect(), LIGHTGRAY);

				if (settings.isOnBox(box)) DrawRectangleLines((int)box.getRect().x, (int)box.getRect().y, (int)box.getRect().width, (int)box.getRect().height, RED);
				else DrawRectangleLines((int)box.getRect().x, (int)box.getRect().y, (int)box.getRect().width, (int)box.getRect().height, DARKGRAY);
		
				DrawText(box.getInput(), (int)box.getRect().x + 5, (int)box.getRect().y + 8, 40, MAROON);
				

				if (settings.isOnBox(box))
				{
					if (box.getLetterCount() < box.MAX_INPUT_CHARS)
					{
						// Draw blinking underscore char
						if (((framesCounter / 20) % 2) == 0) DrawText("_", (int)box.getRect().x + 8 + MeasureText(box.getInput(), 40), (int)box.getRect().y + 12, 40, MAROON);
					}
					else DrawText("Press BACKSPACE to delete chars...", 230, 300, 20, GRAY);
				}
			}
		}
	
	private:
		void drawGameBoard(Game const& game) const {
			float centerX = (sizeConfig.screenWidth - sizeConfig.boardWidth) / 2;
			float centerY = (sizeConfig.screenHeight - sizeConfig.boardHeight) / 2;

			Rectangle gameBoard{ centerX - sizeConfig.boardPadding / 2,centerY - sizeConfig.boardPadding / 2,
								sizeConfig.boardWidth + sizeConfig.boardPadding, sizeConfig.boardHeight + sizeConfig.boardPadding };
			DrawRectangleRec(gameBoard, GRAY);

			for (int i = 0; i < sizeConfig.cols * sizeConfig.rows; ++i) {
				size_t row = i / sizeConfig.cols;
				size_t col = i % sizeConfig.cols;

				TileState state = game.getTileRenderState(row, col);
				Rectangle tileRect = game.getTileRect(row, col);

				switch (state) {
				case TileState::Open: {
					int tileValue = game.getTile(row, col).getValue();
					DrawTexture(tileDownTex, (int)tileRect.x, (int)tileRect.y, { 230,230,230, 255 });
					if (tileValue == 0) 
						break;
					
					DrawText(TextFormat("%i", game.getTile(row, col).getValue()),
						(int)(tileRect.x + (sizeConfig.tileSize / 3)), (int)(tileRect.y + (sizeConfig.tileSize / 4)), 20, getNumberColor(tileValue));
					break;
				}
				case TileState::Bomb:
					DrawTexture(bombTex, (int)tileRect.x, (int)tileRect.y, WHITE);
					break;
				case TileState::Flagged:
					DrawTexture(tileUpTex, (int)tileRect.x, (int)tileRect.y, { 230,230,230, 255 });
					DrawTexture(flagTex, (int)tileRect.x + (int)(tileRect.width- flagTex.width)/2, (int)(tileRect.y) + (int)(tileRect.height - flagTex.height) / 2, WHITE);
					break;
				case TileState::HeldDown:
					DrawTexture(tileDownTex, (int)tileRect.x, (int)tileRect.y, { 230,230,230, 255 });
					break;
				default:
					DrawTexture(tileUpTex, (int)tileRect.x, (int)tileRect.y, {230,230,230, 255});
					break;
				}
			}
		}
		void drawBombCounter(Game const& game)const {
			float centerX = (sizeConfig.screenWidth - sizeConfig.boardWidth) / 2;
			float centerY = (sizeConfig.screenHeight - sizeConfig.boardHeight) / 2;
			float counterWidth = sizeConfig.screenWidth * 0.125f;
			float counterHeight = sizeConfig.screenHeight * 0.075f;
			Vector2 innerPad{ 0.1f * counterWidth, 0.2f * counterHeight };
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
		void drawGameOverMessage(Game const& game)const {
			if (game.getGameState() == GameState::Ongoing)
				return;
			drawMenuButton(game.getHomeButton());

			Vector2 fontPosition = { 0, sizeConfig.screenHeight / 2.0f - GetFontDefault().baseSize / 2.0f - 80.0f };
			if (game.getGameState() == GameState::Won) {
				const char winMsg[10] = "YOU WIN!";
				const char* timeMsg = TextFormat("Time: %ds", (int)game.getGameTime());
				fontPosition.x = (sizeConfig.screenWidth - MeasureText(winMsg, 50)) / 2.0f;
				float center = (MeasureTextEx(GetFontDefault(), winMsg, 50.0f, 5.0f).x - MeasureTextEx(GetFontDefault(), timeMsg, 15.0f, 5.0f).x) / 2;

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
				drawMenuButton(game.getTryAgainButton()); 
			}
			else if (game.getGameState() == GameState::Lost) {
				const char loseMsg[10] = "YOU LOSE!";
				fontPosition.x = (sizeConfig.screenWidth - MeasureText(loseMsg, 50)) / 2.0f;
				Rectangle msgBackground{ fontPosition.x, fontPosition.y, MeasureTextEx(GetFontDefault(), loseMsg, 50.0f,5.0f).x, MeasureTextEx(GetFontDefault(), loseMsg, 50.0f, 5.0f).y };

				//Draw message
				DrawRectangleRounded(msgBackground, 0.1f, 0, DARKGRAY);
				DrawTextEx(GetFontDefault(), loseMsg, fontPosition, 50.0f, 5, BLACK);
				
				
				drawMenuButton(game.getContinueButton());

			}
		}
		void drawMenuButton(Button const& button) const {
			
			Rectangle recSource{ 0.f,0.f, (float)menuBtnTex.width, (float)menuBtnTex.height };
			float centerX = (menuBtnTex.width - MeasureText(button.getText(), 25)) / 2.0f;
			float centerY = (menuBtnTex.height - 25.0f) / 2;
			if (button.isHeldDown())
				DrawTextureRec(menuBtnDownTex, recSource, button.getPosition(), WHITE);
			else
				DrawTextureRec(menuBtnTex, recSource, button.getPosition(), WHITE);

			DrawText(button.getText(), (int)button.getPosition().x + (int)centerX, (int)button.getPosition().y + (int)centerY, 25, button.getTextColor());

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
		
		int framesCounter;
	
		SizeConfig const& sizeConfig;
		Texture2D bombTex, flagTex, tileUpTex, tileDownTex;
		Texture2D menuBtnTex, menuBtnDownTex;
	};
}

class Application {
public:

	Application(SizeConfig& conf) 
		: gameState{ conf }, renderer{ conf }, inputHandler{ conf }, menu{ conf }, settings{ conf }
		, currentScreen{ GameScreen::TITLE } {}

	void update() {
		inputHandler.updateMousePosition(); 
		switch (currentScreen) {
		case GameScreen::TITLE:
			currentScreen = inputHandler.handleMenuInput(menu);
			break;

		case GameScreen::SETTINGS:
			currentScreen = inputHandler.handleSettingsInput(settings, gameState);   // Count frames 

			break;
		case GameScreen::HOW_TO:
			break;
		case GameScreen::GAMEPLAY:
			currentScreen = inputHandler.handleGameInput(gameState);
			break;
		}
	}

	void draw() {
		switch (currentScreen) { 
		case GameScreen::TITLE: 
			renderer.drawMenu(menu); 
			break;
		case GameScreen::SETTINGS: 
			renderer.drawSettings(settings); 
			break;
		case GameScreen::HOW_TO: 
			DrawText(TextFormat("MINESWEEPER"), 0, 0, 50, BLUE); 
			break;
		case GameScreen::GAMEPLAY: 
		{ 
			renderer.drawGame(gameState); 
			DrawFPS(1720, 10); 
		}
		}
	}
	

private:
	GameScreen currentScreen;

	Minesweeper::Renderer renderer;
	Minesweeper::InputHandler inputHandler;

	Minesweeper::Game gameState;
	Minesweeper::Menu menu;
	Minesweeper::Settings settings;
};
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
	SizeConfig sizeConfig{ };
	InitWindow(sizeConfig.screenWidth, sizeConfig.screenHeight, "Minesweeper");
	SetTargetFPS(60);

	Application app{ sizeConfig };


	while (!WindowShouldClose())
	{
		app.update();
		
		// Draw
		BeginDrawing();
		ClearBackground(RAYWHITE);

		app.draw();
		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

    return 0;
}
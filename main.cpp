// Guideline Tetris!!
// V Wheatley

// Code partially based off example project,
// and partially based off my previous Tetris project.

// You may notice me rapidly switching between using `sf::Vector2i` and
// `std::pair<int, int>`. This is because SFML didn't make their vector
// class `constexpr` until.. like a year ago. That change is still only
// in the nightly builds, so... this'll have to do, for specifying points
// in `const` contexts. A bit awkward, as I have to use `.first`/`.second`
// to access the components rather than `.x`/`.y`.

#include <SFML/Graphics.hpp>

#include <vector>
#include <deque>
#include <array>

// TODO: switch RNG to modern C++
#include <time.h>

struct Board {
	const static int WIDTH  = 10;
	const static int HEIGHT = 32;
	
	// The Tetris Guidelines say your board needs to have space beyond the top,
	// but still look like it's 20 tiles tall. It's a neat mechanic! If you try
	// hard enough, you can roll pieces around up to the 24th row and save
	// yourself from game overs!
	const static int VISIBLE_HEIGHT = 20;
	
	const static int TILE_SIZE = 18;
	
	// At the start of the game, the board is filled with empty tiles.
	int board[HEIGHT][WIDTH] = { 0 };
	
	// Clears board of all tiles.
	void clear() {
		for (int j = 0; j < HEIGHT; j++)
			for (int i = 0; i < WIDTH; i++)
				board[j][i] = 0;
	}
	
	// Checks if a line of the board is filled.
	bool isLineFilled(int y) const {
		if (y < 0) return true;
		if (y >= HEIGHT) return false;
		
		for (int i = 0; i < WIDTH; i++)
			if (board[y][i] == 0)
				return false;
		
		return true;
	}
	
	// Removes a line from the board, bringing lines above it down too.
	void removeLine(int y) {
		// TODO: clear line etc.
	}
	
	// Returns screen coordinates of tiles.
	// (Yes, Tetris lives in a +Y-up coordinate space! It's cool)
	static sf::Vector2f getTilePosition(const sf::Vector2i& v) {
		return sf::Vector2f(
			28 + v.x * TILE_SIZE,
			31 + ((VISIBLE_HEIGHT - 1) * TILE_SIZE) - v.y * TILE_SIZE
		);
	}
	
	// Checks if a position is on the board.
	bool isOnBoard(const sf::Vector2i& v) const {
		return v.x >= 0 && v.x < WIDTH && v.y >= 0 && v.y < HEIGHT;
	}
	
	// Returns the bounds-checked tile at the supplied position.
	int getTile(const sf::Vector2i& v) const {
		if (isOnBoard(v)) return board[v.y][v.x];
		return 1;
	}
	
	// Sets the tile at the specified position, only if the position is valid.
	void setTile(const sf::Vector2i& v, int color) {
		if (isOnBoard(v)) board[v.y][v.x] = color;
	}
};

// fun type alias
using PieceRotation = std::array<std::vector<std::pair<int, int>>, 4>;

// Tables from https://harddrop.com/wiki/SRS#How_Guideline_SRS_Really_Works
const PieceRotation PIECE_OFFSETS_I = { {
	{ { 0, 0}, {-1, 0}, {+2, 0}, {-1, 0}, {+2, 0} }, //   0 deg
	{ {-1, 0}, { 0, 0}, { 0, 0}, { 0,+1}, { 0,-2} }, //  90 deg
	{ {-1,+1}, {+1,+1}, {-2,+1}, {+1, 0}, {-2, 0} }, // 180 deg
	{ { 0,+1}, { 0,+1}, { 0,+1}, { 0,-1}, { 0,+2} }  // 270 deg
} };
const PieceRotation PIECE_OFFSETS_JLSTZ { {
	{ { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0} }, //   0 deg
	{ { 0, 0}, {+1, 0}, {+1,-1}, { 0,+2}, {+1,+2} }, //  90 deg
	{ { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0} }, // 180 deg
	{ { 0, 0}, {-1, 0}, {-1,-1}, { 0,+2}, {-1,+2} }  // 270 deg
} };
const PieceRotation PIECE_OFFSETS_O = { {
	{ { 0, 0}, }, //   0 deg
	{ { 0,-1}, }, //  90 deg
	{ {-1,-1}, }, // 180 deg
	{ {-1, 0}, }  // 270 deg
} };

// A piece consists of three things:
struct PieceDefinition {
	// ...a list of tiles (where {0, 0} is the center).
	std::vector<std::pair<int, int>> tiles;
	
	// ...a list of "nudges" to try, in order, to make piece rotation easier
	const PieceRotation* rotations;
	
	// a tile "color" (pretty much just an index into `images/tiles.png`)
	int color;
	
	// Returns the length of the nudge list.
	int getOffsetCheckLength(int prevRotation, int nextRotation) const {
		return std::min(
			rotations->operator[](prevRotation).size(),
			rotations->operator[](nextRotation).size()
		);
	}
	
	// Computes an actual nudge direction, because SRS is bizarre.
	sf::Vector2i getOffset(int prevRotation, int nextRotation, int check) const {
		auto prevOffset = rotations->operator[](prevRotation)[check];
		auto nextOffset = rotations->operator[](nextRotation)[check];
		
		return {
			prevOffset.first - nextOffset.first,
			prevOffset.second - nextOffset.second
		};
	}
};

// List of pieces.
// SCOPE: wouldn't it be cool to define pieces at run time?
const std::vector<PieceDefinition> PIECE_DEFINITIONS = {
	{ { {0, 0}, {-1, 0}, {+1, 0}, {+2, 0} }, &PIECE_OFFSETS_I,     5 }, // I
	{ { {0, 0}, {-1,+1}, {-1, 0}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 7 }, // J
	{ { {0, 0}, {+1,+1}, {-1, 0}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 6 }, // L
	{ { {0, 0}, { 0,+1}, {+1,+1}, {+1, 0} }, &PIECE_OFFSETS_O,     4 }, // O (non-standard)
	{ { {0, 0}, { 0,+1}, {+1,+1}, {-1, 0} }, &PIECE_OFFSETS_JLSTZ, 3 }, // S
	{ { {0, 0}, { 0,+1}, {-1, 0}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 1 }, // T
	{ { {0, 0}, {-1,+1}, { 0,+1}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 2 }  // Z
};

// Rotates a vector in 90 degree increments.
// `rotation` is given in these 90deg increments, so ±2 means 180 degrees.
template<typename T>
sf::Vector2<T> rotate(const sf::Vector2<T>& v, int rotation) {
	// restricts range to 0, 1, 2, 3
	// (or -1, 0, 1, ±2 if you're signed)
	rotation &= 3;
	
	switch (rotation) {
		default: case 0: return { v.x, v.y };
		break; case 1: return { +v.y, -v.x };
		break; case 2: return { -v.x, -v.y };
		break; case 3: return { -v.y, +v.x };
		break;
	}
}

struct Piece {
	// Each piece has a reference to its definition,
	// to retrieve tile color and rotation nudge tables.
	const PieceDefinition* definition;
	
	// Each piece is made up of several tiles relative to its position.
	std::vector<sf::Vector2i> tiles;
	
	// The position of the piece on the board.
	sf::Vector2i position;
	
	// The rotation the piece is at.
	int rotation = 0;
	
	// The piece's initial position on the board.
	static constexpr std::pair<int, int> INITIAL_POSITION = { 4, 20 };
	
	Piece(int id = 0) { reset(id); }
	
	// I'm lazy. This is basically the constructor again.
	void reset(int id = 0) {
		definition = &PIECE_DEFINITIONS[id];
		
		tiles.clear();
		tiles.reserve(definition->tiles.size());
		for (const auto& tile : definition->tiles)
			tiles.push_back({ tile.first, tile.second });
		
		position = { INITIAL_POSITION.first, INITIAL_POSITION.second };
		rotation = 0;
		
		// TODO: nudge piece up if stack high enough. (guideline)
	}
	
	// Attempts to rotate the piece by the specified amount.
	// Returns true if rotation succeeded.
	bool rotate(const Board& board, int direction) {
		int oldRotation = rotation;
		rotation = (rotation + direction) & 3;
		for (auto& tile : tiles) {
			// gosh, eliding `this->` really sucks..
			// i miss rust so much.
			// yes, this is meant to be a call to that template function,
			// not a recursive call.
			tile = ::rotate(tile, direction);
		}
		
		// The core of SRS:
		int checks = definition->getOffsetCheckLength(oldRotation, rotation);
		for (int i = 0; i < checks; i++) {
			auto offset = definition->getOffset(oldRotation, rotation, i);
			
			// Nudge piece in the offset direction.
			if (fitsAt(board, offset)) {
				// If it fits, keep this new position
				// and stop doing further checks.
				position += offset;
				return true;
			}
		}
		// If you get past here, all checks failed.
		
		// Undo rotation if no offsets actually fit.
		rotation = oldRotation;
		for (auto& tile : tiles) {
			tile = ::rotate(tile, -direction);
		}
		
		return false;
	}
	
	// Check if a piece fits on the board, not overlapping any non-zero tile.
	bool fits(const Board& board) const {
		return fitsAt(board, { 0, 0 });
	}
	
	// Check if a piece fits on the board, not overlapping any non-zero tile.
	// But this one accepts an offset!
	bool fitsAt(const Board& board, const sf::Vector2i offset) const {
		for (const auto& tile : tiles)
			if (board.getTile(tile + position + offset) > 0)
				return false;
		return true;
	}
	
	// Writes the piece to the board.
	void place(Board& board) const {
		for (const auto& tile : tiles) {
			board.setTile(tile + position, definition->color);
		}
	}
};

// Piece Randomizer, where every piece has an equal chance of being drawn.
// https://harddrop.com/wiki/Random_Generator
struct PieceBag {
	// If you display the next queue on screen, you need this buffer area--
	// otherwise, every 7 pieces you'd have an empty queue!
	static const int MIN_VISIBLE = 3;
	
	// The range of piece IDs to generate in the RNG.
	static const int PIECES_RANGE = 7 /* PIECE_DEFINITIONS.size() */;
	
	// The number of pieces to append to the queue
	// every time the bag needs to be shuffled.
	static const int BAG_SIZE = PIECES_RANGE;
	// Allows for 14-bag if you want. (I'm just gonna use 7-bag.)
	// static const int BAG_SIZE = PIECES_RANGE * 2;
	
	// The bag!
	std::deque<int> bag;
	
	PieceBag() {
		bag.clear();
		pushNewSet();
	}
	
	// Pops a piece from the front of the queue.
	// (Automatically gets new pieces if 
	int getNext() {
		if (bag.size() <= MIN_VISIBLE)
			pushNewSet();
		
		// oh my god C++ just return the thing you pop from pop_front you jerk
		int result = *bag.cbegin();
		bag.pop_front();
		return result;
	}
	
	// Pushes a new batch of BAG_SIZE pieces to the end of the queue.
	void pushNewSet() {
		std::array<int, BAG_SIZE> shuffle = { 0 };
		
		for (int i = 0; i < shuffle.size(); i++)
			shuffle[i] = i % PIECES_RANGE;
		
		for (int i = shuffle.size() - 1; i > 0; i--) {
			int j = rand() % (i + 1);
			std::swap(shuffle[i], shuffle[j]);
		}
		
		for (const auto& p : shuffle)
			bag.push_back(p);
	}
};

int main() {
	// Seed RNG. This is the old C style, gotta replace that.
	srand(time(0));
	
	// Initialize all the parts of the game.
	// Might give up and make all these global.
	Board board;
	Piece piece;
	
	PieceBag bag;
	piece.reset(bag.getNext());
	
	// Create the dang window.
	sf::RenderWindow window(sf::VideoMode(320, 480), "Tetris");
	window.setVerticalSyncEnabled(true); // Run at a sensible speed.
	
	// Cool pictures
	sf::Texture texTiles, texBackground, texFrame;
	texTiles.loadFromFile("images/tiles.png");
	texBackground.loadFromFile("images/background.png");
	texFrame.loadFromFile("images/frame.png");
	
	sf::Sprite sprTile(texTiles);
	sf::Sprite sprBackground(texBackground);
	sf::Sprite sprFrame(texFrame);
	
	// loose game state
	// input stuff.
	int dx = 0, rotate = 0;
	bool hardDrop = false;
	// fall speed stuff
	float timer = 0, delay;
	
	// Game clock.
	sf::Clock clock;
	sf::Time time;
	
	while (window.isOpen()) {
		// Get delta time.
		auto dt = clock.restart();
		time += dt;
		
		// Reset input state
		dx = 0; rotate = 0; hardDrop = false;
		
		// Poll window & input events.
		sf::Event e;
		while (window.pollEvent(e)) {
			if (e.type == sf::Event::Closed)
				window.close();
			
			// These have key repeat.
			// SCOPE: replace with own DAS system
			if (e.type == sf::Event::KeyPressed) {
				switch (e.key.code) {
					case sf::Keyboard::Z: rotate = -1; break;
					case sf::Keyboard::X: rotate = +1; break;
					case sf::Keyboard::Left:  dx = -1; break;
					case sf::Keyboard::Right: dx = +1; break;
					case sf::Keyboard::Up: hardDrop = true; break;
				}
			}
		}
		
		// This doesn't have key repeat.
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) delay = 0.05;
		else delay = 0.3;

		//// <- Move -> ///
		if (dx != 0) {
			if (piece.fitsAt(board, { dx, 0 }))
				piece.position.x += dx;
		}

		//////Rotate//////
		if (rotate != 0)
			piece.rotate(board, rotate);

		// Fall one tile per tick.
		timer += dt.asSeconds();
		if (timer > delay) {
			if (piece.fitsAt(board, { 0, -1 })) {
				piece.position.y -= 1;
			} else {
				piece.place(board);
				piece.reset(bag.getNext());
			}
			
			timer = 0;
		}

		///////check lines//////////
		// TODO: do it
		// int k = M - 1;
		// for (int i = M - 1; i > 0; i--) {
		// 	int count = 0;
		// 	for (int j = 0; j < N; j++) {
		// 		if (field[i][j]) count++;
		// 		field[k][j] = field[i][j];
		// 	}
		// 	if (count < N) k--;
		// }
		
		/////////draw//////////
		window.clear(sf::Color::White);
		window.draw(sprBackground);
		
		auto setTextureTileIndex = [&sprTile](int i){
			auto r = sf::IntRect(i * Board::TILE_SIZE, 0, Board::TILE_SIZE, Board::TILE_SIZE);
			sprTile.setTextureRect(r);
		};
		
		for (int j = 0; j < Board::HEIGHT; j++) {
			sprTile.setPosition(board.getTilePosition({ -1, j }));
			for (int i = 0; i < Board::WIDTH; i++) {
				sprTile.move(Board::TILE_SIZE, 0);
				
				if (board.board[j][i] == 0) continue;
				
				setTextureTileIndex(board.board[j][i]);
				window.draw(sprTile);
			}
		}
		
		setTextureTileIndex(piece.definition->color);
		for (const auto& tile : piece.tiles) {
			sprTile.setPosition(board.getTilePosition(piece.position + tile));
			window.draw(sprTile);
		}
		
		window.draw(sprFrame);
		window.display();
	}

	return 0;
}

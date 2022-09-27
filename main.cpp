// Guideline Tetris!!
// V Wheatley

// Code partially based off example project,
// and partially based off my previous Tetris project.

#include <SFML/Graphics.hpp>

#include <vector>
#include <array>

#include <time.h>

struct Board {
	const static int WIDTH  = 10;
	const static int HEIGHT = 32;
	
	const static int TILE_SIZE = 18;
	
	int board[HEIGHT][WIDTH] = { 0 };
	
	static auto getTilePosition(const sf::Vector2i& v) -> sf::Vector2f const {
		return sf::Vector2f(28 + v.x * TILE_SIZE, 31 + (19 * TILE_SIZE) - v.y * TILE_SIZE);
	}
	
	int getTile(const sf::Vector2i& v) const {
		if (v.x >= 0 && v.x < WIDTH && v.y >= 0 && v.y < HEIGHT)
			return board[v.y][v.x];
		return 1;
	}
	void setTile(const sf::Vector2i& v, int color) {
		if (v.x >= 0 && v.x < WIDTH && v.y >= 0 && v.y < HEIGHT)
			board[v.y][v.x] = color;
	}
};

using PieceRotation = std::array<std::vector<std::pair<int, int>>, 4>;

const PieceRotation PIECE_NO_OFFSETS = { { {}, {}, {}, {} } };
const PieceRotation PIECE_OFFSETS_I = { {
	{ { 0, 0}, {-1, 0}, {+2, 0}, {-1, 0}, {+2, 0} }, //   0 deg
	{ {-1, 0}, { 0, 0}, { 0, 0}, { 0,+1}, { 0,-2} }, //  90 deg
	{ {-1,+1}, {+1,+1}, {-2,+1}, {+1, 0}, {-2, 0} }, // 180 deg
	{ { 0,+1}, { 0,+1}, { 0,+1}, { 0,-1}, { 0,+2} }  // 270 deg
} };
const PieceRotation PIECE_OFFSETS_JLSTZ { {
	{ { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0} }, //   0 deg
	{ {+1, 0}, {+1,-1}, { 0,+2}, {+1,+2} }, //  90 deg
	{ { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0} }, // 180 deg
	{ {-1, 0}, {-1,-1}, { 0,+2}, {-1,+2} }  // 270 deg
} };
const PieceRotation PIECE_OFFSETS_O = { {
	{ { 0, 0}, }, //   0 deg
	{ { 0,-1}, }, //  90 deg
	{ {-1,-1}, }, // 180 deg
	{ {-1, 0}, }  // 270 deg
} };

using PieceTiles = std::vector<std::pair<int, int>>;

struct PieceDefinition {
	PieceTiles tiles;
	const PieceRotation* rotations;
	int color;
	
	int getOffsetCheckLength(int prevRotation, int nextRotation) const {
		return std::min(
			rotations->operator[](prevRotation).size(),
			rotations->operator[](nextRotation).size()
		);
	}
	
	sf::Vector2i getOffset(int prevRotation, int nextRotation, int check) const {
		auto prevOffset = rotations->operator[](prevRotation)[check];
		auto nextOffset = rotations->operator[](nextRotation)[check];
		
		return {
			prevOffset.first - nextOffset.first,
			prevOffset.second - nextOffset.second
		};
	}
};

const std::vector<PieceDefinition> PIECE_DEFINITIONS = {
	{ { {0, 0}, {-1, 0}, {+1, 0}, {+2, 0} }, &PIECE_OFFSETS_I,     5 }, // I
	{ { {0, 0}, {-1,+1}, {-1, 0}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 7 }, // J
	{ { {0, 0}, {+1,+1}, {-1, 0}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 0 }, // L
	{ { {0, 0}, { 0,+1}, {+1,+1}, {+1, 0} }, &PIECE_OFFSETS_O,     4 }, // O (non-standard)
	{ { {0, 0}, { 0,+1}, {+1,+1}, {-1, 0} }, &PIECE_OFFSETS_JLSTZ, 3 }, // S
	{ { {0, 0}, { 0,+1}, {-1, 0}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 2 }, // T
	{ { {0, 0}, {-1,+1}, { 0,+1}, {+1, 0} }, &PIECE_OFFSETS_JLSTZ, 1 }  // Z
};

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
	const PieceDefinition* definition;
	
	std::vector<sf::Vector2i> tiles;
	
	sf::Vector2i position;
	int rotation = 0;
	
	const sf::Vector2i INITIAL_POSITION = { 4, 20 };
	
	Piece(int id) { reset(id); }
	
	void reset(int id = 0) {
		definition = &PIECE_DEFINITIONS[id];
		
		tiles.clear();
		tiles.reserve(definition->tiles.size());
		for (const auto& tile : definition->tiles) {
			tiles.push_back({ tile.first, tile.second });
		}
		
		position = INITIAL_POSITION;
		rotation = 0;
	}
	
	// Returns true if succeeded.
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
		
		int checks = definition->getOffsetCheckLength(oldRotation, rotation);
		for (int i = 0; i < checks; i++) {
			auto offset = definition->getOffset(oldRotation, rotation, i);
			
			if (fitsAt(board, offset)) {
				position += offset;
				return true;
			}
		}
		
		// Undo rotation if no offsets actually fit.
		rotation = oldRotation;
		for (auto& tile : tiles) {
			tile = ::rotate(tile, -direction);
		}
		
		return false;
	}
	
	bool fits(const Board& board) const {
		return fitsAt(board, { 0, 0 });
	}
	
	bool fitsAt(const Board& board, const sf::Vector2i offset) const {
		for (const auto& tile : tiles)
			if (board.getTile(tile + position + offset) > 0)
				return false;
		return true;
	}
	
	void place(Board& board) const {
		for (const auto& tile : tiles) {
			board.setTile(tile + position, definition->color);
		}
	}
};

int main() {
	Board board;
	Piece piece(1);
	
	srand(time(0));
	
	sf::RenderWindow window(sf::VideoMode(320, 480), "Tetris");
	window.setVerticalSyncEnabled(true);

	sf::Texture t1, t2, t3;
	t1.loadFromFile("images/tiles.png");
	t2.loadFromFile("images/background.png");
	t3.loadFromFile("images/frame.png");
	
	sf::Sprite s(t1), background(t2), frame(t3);

	int dx = 0; int rotate = 0; int colorNum = 1;
	float timer = 0, delay = 0.3; int frames = 0;

	sf::Clock clock;
	sf::Time time;

	while (window.isOpen()) {
		auto dt = clock.restart();
		printf("% 4dms\t", dt.asMilliseconds());
		if (frames % 8 == 7) printf("\n");
		time += dt;
		frames += 1;
		
		sf::Event e;
		while (window.pollEvent(e)) {
			if (e.type == sf::Event::Closed)
				window.close();
			
			// Key repeat
			if (e.type == sf::Event::KeyPressed) {
				switch (e.key.code) {
					case sf::Keyboard::Z: rotate = -1; break;
					case sf::Keyboard::X: rotate = -1; break;
					case sf::Keyboard::Left:  dx = -1; break;
					case sf::Keyboard::Right: dx = +1; break;
				}
			}
		}
		
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) delay = 0.05;

		//// <- Move -> ///
		if (dx != 0) {
			if (piece.fitsAt(board, { dx, 0 }))
				piece.position.x += dx;
		}

		//////Rotate//////
		if (rotate != 0)
			piece.rotate(board, rotate);

		///////Tick//////
		timer += dt.asSeconds();
		if (timer > delay) {
			if (piece.fitsAt(board, { 0, -1 })) {
				piece.position.y -= 1;
			} else {
				piece.place(board);
				piece.reset();
			}
			
			// if (!check()) {
			// 	for (int i = 0; i < 4; i++)
			// 		field[b[i].y][b[i].x] = colorNum;
				
			// 	colorNum = 1 + rand() % 7;
			// 	int n = rand() % 7;
			// 	for (int i = 0; i < 4; i++) {
			// 		a[i].x = figures[n][i] % 2;
			// 		a[i].y = figures[n][i] / 2 + (32 - 20);
			// 	}
			// }
			
			timer = 0;
		}

		///////check lines//////////
		// int k = M - 1;
		// for (int i = M - 1; i > 0; i--) {
		// 	int count = 0;
		// 	for (int j = 0; j < N; j++) {
		// 		if (field[i][j]) count++;
		// 		field[k][j] = field[i][j];
		// 	}
		// 	if (count < N) k--;
		// }
		
		dx = 0; rotate = 0; delay = 0.3;
		
		/////////draw//////////
		window.clear(sf::Color::White);
		window.draw(background);
		
		auto setTextureTileIndex = [&s](int i){ s.setTextureRect(sf::IntRect(i * Board::TILE_SIZE, 0, Board::TILE_SIZE, Board::TILE_SIZE)); };
		
		for (int j = 0; j < Board::HEIGHT; j++) {
			s.setPosition(board.getTilePosition({ -1, j }));
			for (int i = 0; i < Board::WIDTH; i++) {
				s.move(Board::TILE_SIZE, 0);
				
				if (board.board[j][i] == 0) continue;
				
				setTextureTileIndex(board.board[j][i]);
				window.draw(s);
			}
		}
		
		setTextureTileIndex(piece.definition->color);
		for (const auto& tile : piece.tiles) {
			s.setPosition(board.getTilePosition(piece.position + tile));
			window.draw(s);
		}
		
		window.draw(frame);
		window.display();
	}

	return 0;
}

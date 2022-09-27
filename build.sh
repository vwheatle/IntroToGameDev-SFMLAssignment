# note: don't ship the game with this. MSYS2 really insists on linking its own
# standard libraries even if everything else is said to be statically linked.
# this is "good enough" for my comfy development platform, but it's rough for
# everyone else. before turning in the project i need to convert this to a
# vs2019 project or smth.
g++ main.cpp -o blah.exe \
	-D SFML_STATIC \
	-lsfml-graphics-s -lsfml-window-s -lsfml-audio-s -lsfml-system-s \
	-lopenal -lflac -lvorbisenc -lvorbisfile -lvorbis -logg \
	-lgdi32 -lopengl32 -lfreetype -lwinmm

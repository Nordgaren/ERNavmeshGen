# ER Navmesh Gen
A tool to generate navmesh in Elden Ring by turning the exe into a "dll".

# The plan
Eventually `DllMain` will manually map `eldenring.exe` into memory and call the necessary functions to initialize the havok
sdk inside the game. Currently, this project is just looking to initialize the TLS values needed for navmesh generation.
Once we have it working while the game is running normally, we can then do the manual mapping and start initializing the 
parts of the binary necessary for navmesh generation. I want to get the navmesh gen working 

After we get the part of the internal Havok SDK working that we need, I will then make a C# project, which will bind to
the navmesh `ERNavmeshGen` project, so we can use it from C#.

# Relevant code

## Havok::init
These functions were AoB'd before I got the project. This init funciton scans for these AoBs and sets them to their corresponding
static for access later, using the `PATTERN_SCAN` macro. It then initializes our static pointers to the navmesh code I need
to initialize our TLS values on. Since this is a new thread, those variables have to be initialized, and they aren't in the
normal TLS callbacks part of the PE file. https://github.com/Nordgaren/ERNavmeshGen/blob/e57b60f04d9687c417dd58290f709c3ffd6d9e8a/ERNavmeshGen/HavokFunctions.cpp#L51
These offsets will be replaced by AoBs in the future, but for now they work.

## API
The API functions are going to be exports on this DLL, which will handle all the initializing and such. For now the init
function is called in-line, but we will re-assess after it's all working and probably move it to the correct `dwReason` 
in `DLLMain`.

# Project Files
The other project files in this repo

## PE Loader
This is a tool to map a PE from disk into memory so that it can be ran. This includes fixing relocations, etc.

## ERNavmeshGenTests
This is where we will write all the tests for the `ERNavmeshGen` project.

# Future projects
Other projects that will be added

## ERNavmeshGenCS
C# bindings for `ERNavmeshGen`

## ERNavmeshGenCSTests
Tests for `ERNavmeshGenCS`

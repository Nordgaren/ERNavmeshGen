# ER Navmesh Gen
A tool to generate navmesh in Elden Ring by turning the exe into a "dll".

# The plan
Eventually `DllMain` will manually map `eldenring.exe` into memory and call the necessary functions to initialize the havok
sdk inside the game. Currently, this project is just looking to initialize the TLS values needed for navmesh generation.
Once we have it working in a running game, we can then do the manual mapping and start initializing the parts of the binary
necessary for navmesh generation.

After we get the part of the internal Havok SDK working that we need, I will then make a C# project, which will bind to
the navmesh `ERNavmeshGen` project, so we can use it from C#.

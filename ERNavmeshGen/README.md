# ER Navmesh Gen
A tool to generate navmesh in Elden Ring by turning the exe into a "dll".

# The plan
Eventually `DllMain` will manually map `eldenring.exe` into memory and call the necessary functions to initialize the havok
sdk inside the game. Currently this project is just looking to initialize the TLS values needed for navmesh generation.
Once we have it working in a running game, we can then do the manual mapping and start initializing the parts of the binary
necessary for navmesh generation.

I have made considerably more progress on DS3, but have not made any AoBs for either beyond what was provided to me by
12thAvenger and ChurchGuard. 

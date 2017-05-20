# Character Momvement Network Test

This is a more advanced nesnet app. It connects to a server that serves the position of a sprite, allowing control
of it from the other game.

The server source is in the server folder. I may host this on my server, but for now the URL is in `world.c`, and 
has to be manually changed. (Look for a define at the top of the file.) 

You will need to change the IP in `world.c` to the IP of your computer. On windows you can use `ipconfig` to find it. 
Look for one starting with 192.168.1, most likely. 

You can move your sprite by using the control pad. There is no collision logic. Enjoy.
# Character Momvement Network Test

This is a more advanced nesnet app. It connects to a server that serves the position of two sprites, 
allowing control of one from the NES, and another from a web browser.

The server source is in the server folder. I may host this some day, but for now the URL is in `world.c`, and 
has to be manually changed. (Look for a define at the top of the file.) Note that you can also connect to it
with a web browser and move the server sprite yourself!

To get the server running, open a terminal in the `server` folder, then type `npm install`. After modules are
finished installing, type `npm start` to get the process running. From that point you will be able to access
it in your browser at http://localhost:3000.

To make the Rom connect to your server, you will need to change the URL in `world.c` to contain the IP of 
your computer. On startup, the server will list all addresses it knows it is listening on. One of these 
should work - it will often be the one starting with 192.168.1. You will have to be connected to the same 
router as your NES, unless you host this on the internet.

On the NES, you move your character around using the d-pad. On the web client, you can use w/a/s/d, or click
the mouse. There is no collision logic.

Sprites stolen from [Missing Lands](http://cpprograms.net/classic-gaming/missing-lands/), 
my Ludum Dare 38 entry.

Enjoy!
var express = require('express'),
	log = require('winston'),
	app = express(),
	port = 3000,
	// Some pseudo-constants for directions (not used yet)
	DIRECTION_UP = 0,
	DIRECTION_DOWN = 1,
	DIRECTION_LEFT = 2,
	DIRECTION_RIGHT = 3,

	// Player 1 - this one's from the web browser
	p1PositionX = 32,
	p1PositionY = 128,
	p1Direction = DIRECTION_DOWN,

	// Player 2 - this one's from the NES
	p2PositionX = 32,
	p2PositionY = 128,
	p2Direction = DIRECTION_DOWN;;

// Update position for browser player.
function updatePosition(req) {
	p1PositionX = parseInt(req.params.x, 10);
	p1PositionY = parseInt(req.params.y, 10);
	p1Direction = parseInt(req.params.direction, 10);

	if (p1PositionX > 240) 
		p1PositionX = 0;

	if (p1PositionY > 240)
		p1PositionY = 0;

}

// Update position for NES player. 
function updatePositionPost(req) {
	p2PositionX = parseInt(req.body[0], 10);
	p2PositionY = parseInt(req.body[1], 10);
	p2Direction = parseInt(req.body[2], 10);

	if (p2PositionX > 240) 
		p2PositionX = 0;

	if (p2PositionY > 240)
		p2PositionY = 0;

}

// Always parse post requests as raw data.
app.use(require('body-parser').raw({type: function() {  return true; }}));
// Host everything in the public folder as static assets on the /static route
app.use('/static', express.static('public'));

// Get state from the webapp in json form
app.get('/position.json', function(req, res) {
	res.json([
		{x: p1PositionX, y: p1PositionY, direction: p1Direction, character: '@'},
		{x: p2PositionX, y: p2PositionY, direction: p2Direction, character: '#'}
	]);
});

// Get state in a buffer form for the NES.
app.get('/position', function(req, res) {
	res.end(new Buffer([p1PositionX, p1PositionY, p1Direction, p2PositionX, p2PositionY, p2Direction, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]));
});

// Update position and direction from the webapp
app.get('/update/:x/:y/:direction.json', function(req, res) {
	updatePosition(req);
	res.json([
		{x: p1PositionX, y: p1PositionY, direction: p1Direction, character: '@'},
		{x: p2PositionX, y: p2PositionY, direction: p2Direction, character: '#'}
	]);
});

// Update position and direction from the NES
app.post('/update', function(req, res) {
	updatePositionPost(req);
	res.end(new Buffer([p1PositionX, p1PositionY, p1Direction, p2PositionX, p2PositionY, p2Direction, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]));
});

// Get the webapp UI.
app.get('/', function(req, res) {
	res.render('index.pug', {});
});

// Catch-all
app.use(function(req, res, next) {
	// Keep the reply super short to not tie up nesnet.
	res.status(404).end('404');
});

// Finally, just start listening!
app.listen(port, function() {
	log.info("Now listening on the following interfaces: ");
	
	// List all network interfaces so it's easier to figure out what to connect to.
	// Hat tip: https://stackoverflow.com/questions/3653065/get-local-ip-address-in-node-js
	var interfaces = require('os').networkInterfaces();

	Object.keys(interfaces).forEach(function(interfaceName) {
		interfaces[interfaceName].forEach(function(interface) {
			// Only list ipv4 addresses. Don't think we support ipv6, sorry!
			if ('IPv4' !== interface.family) 
				return;

			log.info('  http://' + interface.address + ':' + port + ' (' +interfaceName + ')');
		});
	});
});
var express = require('express'),
	log = require('winston'),
	app = express(),
	port = 3000,
	positionX = 32,
	positionY = 128;

app.get('/update', function(req, res) {
	log.silly("Ran update command");
	positionX += 8;
	if (positionX > 240) {
		positionX = 0;
	}
	res.end(new Buffer([positionX, positionY, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]));
});

// Catch-all
app.use(function(req, res, next) {
	res.status(404).end('404');
});

app.listen(port, function() {
	log.info("Now listening on " + port);
});
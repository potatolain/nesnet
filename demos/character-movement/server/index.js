var express = require('express'),
	log = require('winston'),
	app = express(),
	port = 3000,
	positionX = 32,
	positionY = 128;

function updatePosition(req) {
	positionX = parseInt(req.params.x, 10);
	positionY = parseInt(req.params.y, 10);

	if (positionX > 240) 
		positionX = 0;

	if (positionY > 240)
		positionY = 0;

}

app.get('/position.json', function(req, res) {
	res.json({x: positionX, y: positionY});
});

app.get('/position', function(req, res) {
	res.end(new Buffer([positionX, positionY, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]));
});


app.get('/update/:x/:y.json', function(req, res) {
	updatePosition(req);
	res.json({x: positionX, y: positionY});
});

app.get('/update/:x/:y', function(req, res) {
	updatePosition(req);
	res.end(new Buffer([positionX, positionY, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]));
});


app.get('/', function(req, res) {
	res.render('index.pug', {positionX: positionX, positionY: positionY});
});
app.use('/static', express.static('public'));

// Catch-all
app.use(function(req, res, next) {
	res.status(404).end('404');
});

app.listen(port, function() {
	log.info("Now listening on " + port);
});
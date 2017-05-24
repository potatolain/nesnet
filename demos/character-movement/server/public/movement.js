(function() {

	var MOVEMENT_SPEED = 4,
		DIRECTION_UP = 0,
		DIRECTION_DOWN = 1,
		DIRECTION_LEFT = 2,
		DIRECTION_RIGHT = 3,
		DIRECTION_LOOKUP = {};
	
	DIRECTION_LOOKUP[DIRECTION_UP] = 'up';
	DIRECTION_LOOKUP[DIRECTION_DOWN] = 'down';
	DIRECTION_LOOKUP[DIRECTION_LEFT] = 'left';
	DIRECTION_LOOKUP[DIRECTION_RIGHT] = 'right';

	var positionX = 0,
		positionY = 0,
		direction = DIRECTION_DOWN,
		moveLock = false;

	function getLocationFromServer() {
		$.get('/position.json', function(data) {
			positionX = data.x;
			positionY = data.y;
			direction = data.direction;
			redrawPosition();
		});
	}

	function setLocationOnServer(x, y, direction) {
		if (moveLock)
			return;
		moveLock = true;
		$.get('/update/'+x+'/'+y+'/'+direction+'.json', function(data) {
			moveLock = false;
			positionX = data.x;
			positionY = data.y;
			direction = data.direction;
			redrawPosition();
		});
	}

	function normalizePosition(position) {

		if (position < 0)
			position = 0;
		else if (position > 220)
			position = 220;

		return position;

	}

	function redrawPosition() {
		$('span.positionX').text(positionX);
		$('span.positionY').text(positionY);
		$('span.direction').text(DIRECTION_LOOKUP[direction]);
		$('.panel.world .character').css({left: (2*positionX), top: (2*positionY)});
	}

	$(document).ready(function() {
		getLocationFromServer();

		$('.panel.world .panel-body').click(function(event) {
			var x = event.pageX - $(this).offset().left,
				y = event.pageY - $(this).offset().top;

			x = Math.floor(x/2);
			y = Math.floor(y/2);
			x = normalizePosition(x);
			y = normalizePosition(y);
			setLocationOnServer(x, y, direction);
		});

		$('body').keydown(function(event) {
			switch (event.which) {
				case 87: // w
					direction = DIRECTION_UP;
					positionY -= MOVEMENT_SPEED;
					break;
				case 65: // a
					direction = DIRECTION_LEFT;
					positionX -= MOVEMENT_SPEED;
					break;
				case 83: // s
					direction = DIRECTION_DOWN;
					positionY += MOVEMENT_SPEED;
					break;
				case 68: // d
					direction = DIRECTION_RIGHT;
					positionX += MOVEMENT_SPEED;
					break;

				default: 
					return; // Do nothing!
			}
			positionX = normalizePosition(positionX);
			positionY = normalizePosition(positionY);

			setLocationOnServer(positionX, positionY, direction);
		});
	});
})();
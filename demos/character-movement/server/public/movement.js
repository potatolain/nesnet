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

	var sprites,
		animState = 0,
		p1PositionX = 0,
		p1PositionY = 0,
		direction = DIRECTION_DOWN,
		moveLock = false;

	function getLocationFromServer() {
		$.get('/position.json', function(data) {
			sprites = data;
			p1PositionX = data[0].x;
			p1PositionY = data[0].y;
			direction = data[0].direction;
			redrawPosition();
		});
	}

	function setLocationOnServer(x, y, direction) {
		if (moveLock)
			return;
		moveLock = true;
		$.get('/update/'+x+'/'+y+'/'+direction+'.json', function(data) {
			sprites = data;
			moveLock = false;
			p1PositionX = data[0].x;
			p1PositionY = data[0].y;
			direction = data[0].direction;
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
		$('span.p1PositionX').text(p1PositionX);
		$('span.p1PositionY').text(p1PositionY);
		$('span.direction').text(DIRECTION_LOOKUP[direction]);
		$('.panel.world .character').remove();

		for (var obj in sprites) {
			$('.panel.world .panel-body').append('<div class="character">'+sprites[obj].character+'</div>');
			$('.panel.world .character').last().css({left: (2*sprites[obj].x), top: (2*sprites[obj].y)});
		}
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
					p1PositionY -= MOVEMENT_SPEED;
					break;
				case 65: // a
					direction = DIRECTION_LEFT;
					p1PositionX -= MOVEMENT_SPEED;
					break;
				case 83: // s
					direction = DIRECTION_DOWN;
					p1PositionY += MOVEMENT_SPEED;
					break;
				case 68: // d
					direction = DIRECTION_RIGHT;
					p1PositionX += MOVEMENT_SPEED;
					break;

				default: 
					return; // Do nothing!
			}
			p1PositionX = normalizePosition(p1PositionX);
			p1PositionY = normalizePosition(p1PositionY);

			setLocationOnServer(p1PositionX, p1PositionY, direction);
		});

		setInterval(function() {
			getLocationFromServer();
		}, 2000);

		setInterval(function() {
			animState = ! animState;
			if (animState) {
				$('.panel.world').addClass('anim1').removeClass('anim2');
			} else {
				$('.panel.world').addClass('anim2').removeClass('anim1');
			}
		}, 500);
	});
})();
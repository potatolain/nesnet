(function() {

	var positionX = 0,
		positionY = 0;

	function getLocationFromServer() {
		$.get('/position.json', function(data) {
			positionX = data.x;
			positionY = data.y;
			redrawPosition();
		});
	}

	function setLocationOnServer(x, y) {
		$.get('/update/'+x+'/'+y+'.json', function(data) {
			positionX = data.x;
			positionY = data.y;
			redrawPosition();
		});
	}

	function redrawPosition() {
		$('span.positionX').text(positionX);
		$('span.positionY').text(positionY);
		$('.panel.world .character').css({left: (2*positionX), top: (2*positionY)});
	}

	$(document).ready(function() {
		getLocationFromServer();

		$('.panel.world .panel-body').click(function(event) {
			var x = event.pageX - $(this).offset().left,
				y = event.pageY - $(this).offset().top;

			x = Math.floor(x/2);
			y = Math.floor(y/2);
			setLocationOnServer(x, y);
		});
	});
})();
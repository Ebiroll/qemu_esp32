//LEGACY CODE. HAS BEEN SUPERCEDED BY demoClass.js

//thanks to http://stackoverflow.com/questions/11101364/javascript-detect-shift-key-down-within-another-function
var shiftDown = false;
var setShiftDown = function(event){
	if(event.keyCode === 16 || event.charCode === 16){ //for future reference, alt key is 18
		shiftDown = true;
	}
};

var setShiftUp = function(event){
	if(event.keyCode === 16 || event.charCode === 16){
		shiftDown = false;
	}
};

function isNumber(n) {
	return !isNaN(parseFloat(n)) && isFinite(n);
}

$(document).on("keydown", function(e){
	setShiftDown(e);
});

$(document).on("keyup", function(e){
	setShiftUp(e);
});

$(document).on("scriptLoaded", function(){

	if (typeof(ui) !== "undefined"){
		for (var prop in ui){

			(function(prop){

				propContainerSelector = '#'+prop+'-interface'; 

				if (ui[prop].className){
					className = ui[prop].className + " ";
				} else {
					className = '';
				}
				$('#ui-container').append("<div class='interface " +className+ "clearfix' id='"+prop+"-interface'></div>");

				//buttons don't need <label> tags because their "label" is determined like so: <button>Label</button>
				if (ui[prop].type != "button"){
					$(propContainerSelector).append("<label>"+ui[prop].title+"</label>");
				}

			  	if (isNumber(ui[prop].value) && (!$.isArray(ui[prop].values))){ 
			  		if (ui[prop].units){
			  			sliderInputBoxHTML = "<div class='input-group'><input class='form-control with-units' value='"+ui[prop].value+"'><span class='input-group-addon'>"+ui[prop].units+"</span></div>";
			  		} else if (ui[prop].input === 'readonly'){
			  			sliderInputBoxHTML = "<input value='"+ui[prop].value+"' readonly>";
			  		} else if (ui[prop].input === 'hidden') {
			  			sliderInputBoxHTML = "<input class='form-control' value='"+ui[prop].value+"' type='hidden'>";
			  		} else {
			  			sliderInputBoxHTML = "<input class='form-control' value='"+ui[prop].value+"'>";
			  		}

			  		$(propContainerSelector).append(sliderInputBoxHTML);

					$(propContainerSelector).noUiSlider({
						range: ui[prop].range,
						start: ui[prop].value,
						handles: 1,
						connect: "lower",
						step: (ui[prop].step) ? ui[prop].step : undefined,
						slide: function(){
							ui[prop].value = parseFloat($(this).val());
							update(prop);
						},
						change: function(){
							ui[prop].value = parseFloat($(this).val());
							update(prop);
						},
						set: function(){
							ui[prop].value = parseFloat($(this).val());
							update(prop);
							ga('send', 'event', ui[prop].title, 'slide', window.location.pathname);
						},
						serialization: {
							to: (ui[prop].input !== 'hidden' || ui[prop].input !== 'readonly') ? [$('#'+prop+'-interface input')] : [false, false],
							resolution: ui[prop].resolution
						}
					});


					//Keyboard increment
					$('#'+prop+'-interface input').keydown(function(e){

						var value = parseInt($(propContainerSelector).val());
						var increment = shiftDown ? 10 : 1;

						switch (e.which){
							case 38:
								$(propContainerSelector).val( value + increment );
								ui[prop].value = parseFloat($(this).val());
							    ga('send', 'event', ui[prop].title, 'increment: +'+increment, window.location.pathname);
								break;
						    case 40:
							    $(propContainerSelector).val( value - increment );
							    ui[prop].value = parseFloat($(this).val());				    
							    ga('send', 'event', ui[prop].title, 'decrement: -'+increment, window.location.pathname);
							    break;
						}

						update(prop);

					});

					//set color
					if (ui[prop].color){
						$('#'+prop+'-interface .noUi-connect').css("background-color", ui[prop].color);
					}

				} else if (ui[prop].value === true || ui[prop].value === false) {

				    $('#'+prop+'-interface label').attr("for", prop+'-checkbox');

				    initialCheckboxSetting = ui[prop].value === true ? "checked" : "";

				    $(propContainerSelector).append("<div class='checkbox'><input type='checkbox' value='None' id='"+prop+"-checkbox' name='check' "+initialCheckboxSetting+" /><label for='"+prop+"-checkbox'></label></div>");

				    $('#'+prop+'-interface input').change(function(){
				    	if ($(this).prop('checked')){
				    		ui[prop].value = true;
				    		eventLabel = 'checkbox: switch on'
					    } else {
					    	ui[prop].value = false;
					    	eventLabel = 'checkbox: switch on'
					    }
				        ga('send', 'event', ui[prop].title, eventLabel, window.location.pathname);
					    update(prop);
					});
				} else if ($.isArray(ui[prop].values)){
					//Dropdown Menus
					$(propContainerSelector).append("<select class='form-control'></select");

					for (var i  = 0 ; i < ui[prop].values.length ; i++){
						$('#'+prop+'-interface select').append("<option value='"+ui[prop].values[i][1]+"'>"+ui[prop].values[i][0]+"</option>");
				    }

					$('#'+prop+'-interface select option[value="'+ui[prop].value+'"]').prop('selected', true);

				    $('#'+prop+'-interface select').change(function(){
				    	ui[prop].value = $(this).val();
				    	ga('send', 'event', ui[prop].title, 'Dropdown change: ' + ui[prop].value, window.location.pathname);
				    	$('#'+prop+'-interface select option')
					    	.prop('selected', false)
					    	.filter('[value="'+ui[prop].value+'"]').prop('selected', true);
				    	update(prop);
				    })

				} else if (ui[prop].type == "button"){
					$(propContainerSelector).append("<button>"+ui[prop].title+"</button>").click(function(){
						update(prop);
					});
				} else {
					$(propContainerSelector).append("<input value='"+ui[prop].value+"' readonly>");
				}
			})(prop);

		}
	}

	$("body").on('click', '#ui-container a', function(e){
		//sends data about clicking links in #ui-container. Such as in nuclear crater map
		ga('send', 'event', $(this).html(), 'click', window.location.pathname);
	});

	$(document).trigger({
		type: "uiLoaded"
	});

});
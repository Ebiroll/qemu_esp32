


function isNumber(n) {
	return !isNaN(parseFloat(n)) && isFinite(n);
}


function Demo(settings){
	var self = this;

	for (var name in settings) {
	    this[name] = settings[name];
	}
	
	this.ui = typeof this.ui === "undefined" ? {} : this.ui;

	this.shiftDown = false;

	//thanks to http://stackoverflow.com/questions/11101364/javascript-detect-shift-key-down-within-another-function
	this.setShiftDown = function(event){
		if(event.keyCode === 16 || event.charCode === 16){ //for future reference, alt key is 18
			self.shiftDown = true;
		}
	};

	this.setShiftUp = function(event){
		if(event.keyCode === 16 || event.charCode === 16){
			self.shiftDown = false;
		}
	};

	this.addUIElement = function(prop){
		var ui = this.ui;
		// console.log(this, ui);

		var propContainerSelector = '#'+prop+'-interface'; 

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

		if (ui[prop].type == "userInputNumerical"){
  			inputBoxHTML = "<input class='form-control user-input-numerical' value='"+ui[prop].value+"'>";
	  		$(propContainerSelector).append(inputBoxHTML);
  		    $('#'+prop+'-interface input').change(function(){
	    		ui[prop].value = parseFloat($('#'+prop+'-interface input').val());
  		        self.sendEvent(ui[prop].title, 'value changed', window.location.pathname);
  			    // self.update(prop);
			});
		} else if (ui[prop].type == "userInputString"){
			var inputBoxHTML = "";
  			if (ui[prop].prepend){
	  			inputBoxHTML = "<div class='input-group'>";
  				inputBoxHTML += "<span class='input-group-addon'>"+ui[prop].prepend+"</span>";
  			}
  			inputBoxHTML += "<input class='form-control user-input-string' value='"+ui[prop].value+"'>";
  			if (ui[prop].prepend){
	  			inputBoxHTML += "</div>";
	  		}
	  		$(propContainerSelector).append(inputBoxHTML);
  		    $('#'+prop+'-interface input').change(function(){
	    		ui[prop].value = $('#'+prop+'-interface input').val();
  		        self.sendEvent(ui[prop].title, 'value changed', window.location.pathname);
  			    // self.update(prop);
			});
		} else if (ui[prop].type == "userInputTextarea"){
  			inputBoxHTML = "<textarea class='form-control user-input-textarea'>"+ui[prop].value+"</textarea>";
	  		$(propContainerSelector).append(inputBoxHTML);
  		    $('#'+prop+'-interface textarea').change(function(){
	    		ui[prop].value = $('#'+prop+'-interface textarea').val();
  		        self.sendEvent(ui[prop].title, 'value changed', window.location.pathname);
  			    // self.update(prop);
			});
		} else if (isNumber(ui[prop].value) && (!$.isArray(ui[prop].values))){ 
			//console.log("Adding slider!");
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
					self.update(prop);
					if ($('#'+prop+'-interface input').val() === "-0"){
						$('#'+prop+'-interface input').val("0");
					}
					// self.update(prop);
				},
				change: function(){
					ui[prop].value = parseFloat($(this).val());
					self.update(prop);
				},
				set: function(){
					ui[prop].value = parseFloat($(this).val());
					self.update(prop);
					self.sendEvent(ui[prop].title, 'slide', window.location.pathname);
				},
				serialization: {
					to: (ui[prop].input !== 'hidden' || ui[prop].input !== 'readonly') ? [$('#'+prop+'-interface input')] : [false, false],
					resolution: ui[prop].resolution
				}
			});


			//Keyboard increment
			$('#'+prop+'-interface input').keydown(function(e){

				var value = parseInt($(propContainerSelector).val());
				var increment = self.shiftDown ? 10 : 1;

				switch (e.which){
					case 38:
						$('#'+prop+'-interface input').val( value + increment );
						ui[prop].value = parseFloat($(this).val());
					    self.sendEvent(ui[prop].title, 'increment: +'+increment, window.location.pathname);
						break;
				    case 40:
					    $('#'+prop+'-interface input').val( value - increment );
					    ui[prop].value = parseFloat($(this).val());				    
					    self.sendEvent(ui[prop].title, 'decrement: -'+increment, window.location.pathname);
					    break;
				}

				self.update(prop);

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
					console.log("ON")
			    } else {
			    	ui[prop].value = false;
					eventLabel = 'checkbox: switch on'
					console.log("ON2")
			    }
		        self.sendEvent(ui[prop].title, eventLabel, window.location.pathname);
			    self.update(prop);
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
		    	self.sendEvent(ui[prop].title, 'Dropdown change: ' + ui[prop].value, window.location.pathname);
		    	$('#'+prop+'-interface select option')
			    	.prop('selected', false)
			    	.filter('[value="'+ui[prop].value+'"]').prop('selected', true);
		    	self.update(prop);
		    })

		} else if (ui[prop].type == "button"){
			$(propContainerSelector).append("<button>"+ui[prop].title+"</button>").click(function(){
				self.update(prop);
			});
		} else {
			$(propContainerSelector).append("<input value='"+ui[prop].value+"' readonly>");
		}
	}

	for (var prop in this.ui){
		this.addUIElement(prop);
	}

	//sends data about clicking links in #ui-container. Such as in nuclear crater map
	$("body").on('click', '#ui-container a', function(e){
		self.sendEvent($(this).html(), 'click', window.location.pathname);
	});


	$(document).on("keydown", function(e){
		self.setShiftDown(e);
	});

	$(document).on("keyup", function(e){
		self.setShiftUp(e);
	});

	this.sendEvent = function(category, action, label, value){
		console.log(label,value);
		if (window.location.host == 'academo.org'){
			//ga('send', 'event', category, action, label, value);
		}
	}


	this.init();
}
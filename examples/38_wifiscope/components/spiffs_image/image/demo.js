var ui = {
    inputType: {
        title: "Input",
        value: 2,
        values: [["Live audio Input (5 V peak amplitude)",1], ["Sine Wave (amplitude 5 V)",2], ["Square Wave (amplitude 5 V)",3], ["http (esp32)",4]]
    },
    attenuation: {
        title: "Attenuation",
        value: 0,
        values: [["ADC_ATTEN_0db, full voltage 1.1V",0],["ADC_ATTEN_2_5db, full 1.5V",1],["ADC_ATTEN_6db, full 2.2V",2],["ADC_ATTEN_11db, full voltage 3.9V",3]]
    },

    bits: {
        title: "Capture width",
        value: 0,
        values: [["9 bits",0],["10 bits",1],["11 bits",2],["12 bits",3]]
    },

    ccount: {
        title: "ccount_delay",
        value: 0,
        values: [["Min delay",0],["8k",8000],["80k",80000],["800k",80000]]
    },


    //ADC1_CHANNEL_0 = 0
    //ADC1 channel 0 is GPIO36
    
    //ADC1_CHANNEL_1
    //ADC1 channel 1 is GPIO37
    
    //ADC1_CHANNEL_2
    //ADC1 channel 2 is GPIO38
    
    //ADC1_CHANNEL_3
    //ADC1 channel 3 is GPIO39
    
    //ADC1_CHANNEL_4
    //ADC1 channel 4 is GPIO32
    
    //ADC1_CHANNEL_5
    //ADC1 channel 5 is GPIO33
    
    //ADC1_CHANNEL_6
    //ADC1 channel 6 is GPIO34
    
    //ADC1_CHANNEL_7
    //ADC1 channel 7 is GPIO35

    fdomain: {
        title: "Frequency domain",
        value: false,
    },

    freeze: {
        title: "Freeze Live Input",
        value: false,
    },
    freq: {
        title: "Input Wave Frequency",
        value: 250,
        range:[1,1000],
        resolution:1,
        units: "Hz"
    },
    gain: {
        title: "Oscilloscope gain",
        value: 1,
        range:[0,5],
        resolution:0.1,
    },
    // invert: {
    //     title: "Invert",
    //     value: false,
    // },
    dropdownExample: {
        title: "Seconds / div",
        value: 1,
        values: [["50 µs", 0.05],["100 µs", 0.1],["200 µs", 0.2],["500 µs", 0.5],["1 ms", 1], ["2 ms", 2],["5 ms", 5]]
    },
    volts: {
        title: "Volts / div",
        value: 1,
        values: [["0.1 V", 0.02],["0.5 V", 0.1],["1 V", 0.2],["10 V", 2]]
    },
    // dc_offset: {
    //     title: "Vertical Offset",
    //     value: 0,
    //     range:[-300,300],
    //     resolution:0.1,
    //     input: "hidden"
    // },
    horizOffset: {
        title: "Horizontal Offset",
        value: 0,
        range:[-100,100],
        resolution:1,
        input: "hidden"
    },
    vertOffset: {
        title: "Vertical Offset",
        value: 0,
        range:[-100,100],
        resolution:1,
        input: "hidden"
    }

};

$(document).on("uiLoaded", function(){
    if (navigator.getUserMedia = (navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia || navigator.msGetUserMedia)){
        navigator.getUserMedia( {audio:true}, gotStream,function(error) {
            console.log("Capture error: ", error.code);
          });
    } else {
        animate();
        $(".preamble").append("<div class='alert'>To use Live Audio Input, please download the latest version of Chrome.</div>");
        $("#inputType-interface option[value=1]").attr("disabled", true);
    };

});



function getQueryVariable(variable) {
  var query = window.location.search.substring(1);
  var vars = query.split("&");
  for (var i=0;i<vars.length;i++) {
    var pair = vars[i].split("=");
    if (pair[0] == variable) {
      return pair[1];
    }
  } 
}



demo = document.getElementById('demo');
c = document.createElement("canvas"); // for gridlines
c2 = document.createElement("canvas"); // for animated line
var w = window;
screenHeight = w.innerHeight;
screenWidth = w.innerWidth;
c.width = document.body.clientWidth*0.8;
//c.width = demo.clientWidth;
c.height = document.body.clientHeight;
c.height = c.width * 0.57;
c2.width = document.body.clientWidth*0.8;
//c2.width = demo.clientWidth;
c2.height = document.body.clientHeight;
c2.height = c.height;
$("#demo").height((c.height + 20));
c.style.backgroundColor = "#5db1a2";
demo.appendChild(c);
demo.appendChild(c2);

midPoint = {x: c.width/2, y: c.height/2};

ctx = c.getContext("2d");
ctx2 = c2.getContext("2d");

function createGrid(ctx){
    ctx.beginPath();
    ctx.moveTo(0, midPoint.y);
    ctx.lineTo(c.width, midPoint.y);
    ctx.moveTo(midPoint.x, 0);
    ctx.lineTo(midPoint.x, c.height);
    ctx.strokeStyle = "#196156";
    ctx.lineWidth = '2';
    ctx.globalCompositeOperation = 'source-over';
    ctx.stroke();
    ctx.closePath();

    ctx.beginPath();
    gridLineX = midPoint.x - 100;
    ctx.lineWidth = '2';
    while (gridLineX >= 0){
      ctx.moveTo(gridLineX, 0);
      ctx.lineTo(gridLineX, c.height);
      gridLineX -= 100;
  }
  gridLineX = midPoint.x + 100;
  while (gridLineX <= c.width){
      ctx.moveTo(gridLineX, 0);
      ctx.lineTo(gridLineX, c.height);
      gridLineX += 100;
  }
  gridLineY = midPoint.y - 100;
  while (gridLineY >= 0){
      ctx.moveTo(0, gridLineY);
      ctx.lineTo(c.width, gridLineY);

      gridLineY -= 100;
  }
  gridLineY = midPoint.y + 100;
  while (gridLineY <= c.height){
      ctx.moveTo(0, gridLineY);
      ctx.lineTo(c.width, gridLineY);
      gridLineY += 100;
  }
  dashesX = midPoint.x - 20;
  while (dashesX >= 0){
      ctx.moveTo(dashesX, midPoint.y-5);
      ctx.lineTo(dashesX, midPoint.y+5);
      dashesX -= 20;
  }
  while (dashesX <= c.width){
      ctx.moveTo(dashesX, midPoint.y-5);
      ctx.lineTo(dashesX, midPoint.y+5);
      dashesX += 20;
  }
  dashesY = midPoint.y - 20;
  while (dashesY >= 0){
      ctx.moveTo(midPoint.x-5, dashesY);
      ctx.lineTo(midPoint.x+5, dashesY);
      dashesY -= 20;
  }
  dashesY = midPoint.y + 20;
  while (dashesY <= c.height){
      ctx.moveTo(midPoint.x-5, dashesY);
      ctx.lineTo(midPoint.x+5, dashesY);
      dashesY += 20;
  }

  ctx.stroke();

}

createGrid(ctx);

var isRunning = false;

function update(el){

  if (el == 'inputType' && ui.inputType.value == 1){
      streaming = true;
      animate();
      animateId = window.requestAnimationFrame(animate);

  } else if (el == 'inputType' && ui.inputType.value != 1){
    //cancel animation
    streaming = false;
    window.cancelAnimationFrame(animateId);
    drawData();
  } else if (streaming == true && ui.freeze.value == false){
      animate();
  } 
  else {
    drawData();
  }

    //only need to 'animate' one frame when not isRunning
    // console.log(ui.freeze.value);
    // if (streaming != true){
    //     if (isRunning == false){
    //         isRunning = true;
    //         streaming = true;
    //         animate();
    //         // drawData();
    //     } else {
    //       streaming = false;
    //         drawData();
    //     }
    // }

    // if (el.title == "Freeze"){
    //     if (ui.freeze.value){
    //         window.cancelAnimationFrame(animateId);
    //     } else {
    //         animate();
    //         // drawData();
    //     }
    // }




    // if (ui.freeze.value && isRunning){
    //     // isRunning = false;
    //     window.cancelAnimationFrame(animateId);
    // } else if(ui.freeze.value && !isRunning) {

    // } else if (!ui.freeze.value && !isRunning){
    //     isRunning = true;
    //     animate();
    // }

}



var AudioContext = (window.AudioContext || window.webkitAudioContext || window.mozAudioContext || window.oAudioContext || window.msAudioContext);

if (AudioContext){
  var audioContext = new AudioContext();
  // var gainNode = audioContext.createGain() || audioContext.createGainNode();
  var gainNode = audioContext.createGain();
  var analyser = audioContext.createAnalyser();

  //confusing, gain on oscilloscope, different for gain affecting input
  // gainNode.gain.value = ui.gain.value;

  gainNode.gain.value = 3;


  analyser.smoothingTimeConstant = .9;
  // analyser.fftSize = 512;
  // analyser.fftSize = 1024;
  // analyser.fftSize = 4096;
  try {
    analyser.fftSize = 4096;
  } catch(e) {
    analyser.fftSize = 2048;
  }
  gainNode.connect(analyser);
  // frequencyBinCount is readonly and set to fftSize/2;
  var timeDomain = new Uint8Array(analyser.frequencyBinCount);
  var streaming = false;
  var sampleRate = audioContext.sampleRate;
  var numSamples = analyser.frequencyBinCount;
} else {
  var analyser = {};
  analyser.frequencyBinCount = 512;
}




function gotStream(stream) {
    // Create an AudioNode from the stream.
    window.mediaStreamSource = audioContext.createMediaStreamSource( stream );

    //for testing
    var osc = audioContext.createOscillator();
    osc.frequency.value = 200;
    osc.start(0);

    // switch these lines
    window.mediaStreamSource.connect(gainNode);
    // osc.connect(gainNode);

    streaming = true;
    $('#inputType-interface select').val(1).change();
    animate();
}

$(document).on("change", '#inputType-interface select', function(){
    if ($(this).val() == 1){
        streaming = true;
        $('#freq-interface').attr('disabled', 'disabled').addClass("disabled");
        $('#fdomain-interface').removeAttr('disabled').removeClass("disabled"); 
    } else {
        streaming = false;
        $('#freq-interface').removeAttr('disabled').removeClass("disabled");
        $('#fdomain-interface').attr('disabled', 'disabled').addClass("disabled");

    }

    if ($(this).val() == 4){
        streaming = true;
        $('#freq-interface').attr('disabled', 'disabled').addClass("disabled");
        $('#fdomain-interface').removeAttr('disabled').removeClass("disabled"); 
    } 

})


var mapRange = function(from, to, s) {
  return to[0] + (s - from[0]) * (to[1] - to[0]) / (from[1] - from[0]);
};


var animateId;
var previousTranslate = {x:0, y:0};

function animate(){

    if (streaming == true && ui.freeze.value == false){
        // OLAS !!!         
        if (ui.fdomain.value == true) {
            analyser.getByteFrequencyData(timeDomain);
        } else {
            analyser.getByteTimeDomainData(timeDomain);
        }
        window.requestAnimationFrame(animate);
    }

    drawData();
    // gainNode.gain.value = ui.gain.value;
}

function drawData(){
    ctx2.translate(-previousTranslate.x, -previousTranslate.y);
    ctx2.clearRect(0,0,c.width,c.height);
    ctx2.translate(ui.horizOffset.value, ui.vertOffset.value);
    ctx2.beginPath();
    ctx2.strokeStyle = '#befde5';
    ctx2.lineWidth = 1;

    for (var i = -analyser.frequencyBinCount/2; i <= analyser.frequencyBinCount/2; i++) {
        index = i+analyser.frequencyBinCount/2;
        // console.log(index);



        if (streaming == true){

            var height = c.height * timeDomain[i] / 256;
            var offset = c.width * (analyser.frequencyBinCount/(analyser.frequencyBinCount-1)) * i/analyser.frequencyBinCount;

            // analyser.getByteTimeDomainData(timeDomain);


            var xc = i * (c.width/analyser.frequencyBinCount);
            var yc = ui.gain.value * ((timeDomain[index] / 255) - 0.5)*200/(ui.volts.value);

            yc += c.height/2;

            // apply dc offset
            //yc = ui.dc_offset.value*-1 + yc;

            xc = mapRange([0, 0.001*ui.dropdownExample.value], [0, 100 * (numSamples/sampleRate) / c.width], xc);

            // shift graph to middle of oscilloscpe
            xc += c.width/2;


            ctx2.lineTo(xc, yc);

        } else {

            var xc = i * (c.width/analyser.frequencyBinCount);

            //dropdown value also needs mapping
            scaledRangeValue = mapRange([1,2], [1,3], ui.dropdownExample.value);

            //Hardcoding 6 is wrong! Gives incorrect values on small screens
            // var amplitude = c.height/6 / ui.volts.value;
            var amplitude = 100 / ui.volts.value; //100 pixels per division

            //so total length in seconds of graph is sampleRate*numSamples
            //console.log("total length:", numSamples/sampleRate); //=0.0053333seconds, so 1 pixel represents 0.00533/width seconds
            //by default 100px represents 100*0.00533/width  = ??? seconds
            //we want it to represent 1ms


            var yc =  -amplitude * Math.sin(2*Math.PI*xc*ui.freq.value*0.00001*ui.dropdownExample.value); //0.00001 is the number of seconds we want a pixel to represent, ie 1ms / 100

            if (ui.inputType.value == 3){
              if (yc > 0) yc = amplitude;
              else yc = -amplitude;
            }


            //apply gain
            yc *= ui.gain.value;

            //center vertically
            yc = c.height/2 + yc;

            // apply dc offset
            //yc = ui.dc_offset.value*-1 + yc;

            // shift graph to middle of oscilloscpe
            xc += c.width/2;

            // if (ui.invert.value) yc = -yc + c.height;

            ctx2.lineTo(xc, yc);

        }

        previousTranslate = {x:ui.horizOffset.value,y:ui.vertOffset.value}

    }

    ctx2.stroke();
    ctx2.strokeStyle = 'rgba(174,244,218,0.3)';
    ctx2.lineWidth = 3;
    ctx2.stroke();
    ctx2.strokeStyle = 'rgba(174,244,218,0.3)';
    ctx2.lineWidth = 4;
    ctx2.stroke();

}

animate();
$(function() {
	var ESP32_IP = "192.168.1.99";
	
	function getData(path, callback) {
		jQuery.ajax({
			url : "http://" + ESP32_IP + ":80" + path,
			method : "GET",
			dataType : "json",
			success : function(data, textStatus, jqXHR) {
				if (callback) {
					callback(data);
				}
			},
			error : function(jqXHR, textStatus, errorThrown) {
				console.log("AJAX error");
				debugger;
			}
		});
	} // getData

	
	function postData(path, callback) {
		jQuery.ajax({
			url : "http://" + ESP32_IP + ":80" + path,
			method : "POST",
			dataType : "json",
			success : function(data, textStatus, jqXHR) {
				if (callback) {
					callback(data);
				}
			},
			error : function(jqXHR, textStatus, errorThrown) {
				console.log("AJAX error");
				debugger;
			}
		});
	} // postData

	
	function showGPIODialog(gpio) {
		$("#gpioDialog").dialog("option", {
			title: "GPIO " + gpio
		});
		getData("/ESP32/GPIO", function(data) {
			$("#gpioJsonText").val(JSON.stringify(data, null, "  "));

			$("#gpioDialog_OUT_REG").text(data.out[gpio]);
			$("#gpioDialog_IN_REG").text(data.in[gpio]);
			$("#gpioDialog_ENABLE_REG").text(data.enable[gpio]);
			$("#gpioDialog_STATUS_REG").text(data.enable[gpio]);
			if (data.enable[gpio]) {
				$("#inputGpioDialog").prop("checked", false).button("refresh");
				$("#outputGpioDialog").prop("checked", true).button("refresh");
			} else {
				$("#inputGpioDialog").prop("checked", true).button("refresh");
				$("#outputGpioDialog").prop("checked", false).button("refresh");
			}
			$("#inputGpioDialog").attr("data-gpio", gpio);
			$("#outputGpioDialog").attr("data-gpio", gpio);
			$("#gpioDialog").dialog("open");
		})
	} // showGPIODialog

	
	function typeToString(type) {
		switch (type) {
		case 0x00:
			return "App";
		case 0x01:
			return "Data";
		default:
			return type.toString();
		}
	} // typeToString

	function subTypeToString(subtype) {
		switch (subtype) {
		case 0x00:
			return "Factory";
		case 0x01:
			return "Phy";
		case 0x02:
			return "NVS";
		case 0x03:
			return "Coredump";
		case 0x80:
			return "ESP HTTPD";
		case 0x81:
			return "FAT";
		case 0x82:
			return "SPIFFS";
		default:
			return subtype.toString();
		}
	}

	console.log("ESP32Explorer loading");
	$("#myTabs").tabs();
	$("#systemTabs").tabs();
	$("#gpioTabs").tabs();
	$("#wifiTabs").tabs();
	$("#systemLoggingTab [name='systemLogging']").checkboxradio().on("change", function(event) {
		postData("/ESP32/LOG/SET/" + $(event.target).attr("data-logLevel"));
	});
	
	$("#inputGpioDialog").checkboxradio().on("change", function(event) {
		var gpio = $(event.target).attr("data-gpio");
		postData("/ESP32/GPIO/DIRECTION/INPUT/" + gpio);
		//debugger;
	});
	$("#outputGpioDialog").checkboxradio().on("change", function(event) {
		var gpio = $(event.target).attr("data-gpio");
		postData("/ESP32/GPIO/DIRECTION/OUTPUT/" + gpio);
		//debugger;
	});
	
	// Draw the GPIO table.
	var table = $("#gpioTable");
	for (var j=0; j<5; j++) {
		var tr = $("<tr>");
		for (var i=0; i<8; i++) {
			var gpioNum = j*8 + i;
			var td = $("<td>");
			var div;
			var wDiv;
			wDiv = $("<div class='flexHorizCenter'>");
			div = $("<div>");
			div.attr("id", "gpio" + gpioNum + "Icon");
			wDiv.append(div);
			
			div = $("<div class='flexHorizCenter' style='margin-left: 4px; font-size: x-large;'>");
			div.text("" + gpioNum);
			wDiv.append(div);
			
			td.append(wDiv);
			
			wDiv = $("<div class='flexHorizCenter'>");
			var checkBox = $("<input type='checkbox'>");
			checkBox.attr("data-gpio", gpioNum );
			checkBox.click(function(event){
				var gpio = event.target.dataset.gpio;
				var state = event.target.checked;
				console.log("State of " + gpio + " now " + state);
				if (state) {
					postData("/ESP32/GPIO/SET/" + gpio);
				} else {
					postData("/ESP32/GPIO/CLEAR/" + gpio);
				}
			});
			checkBox.attr("id", "gpio" + gpioNum + "Checkbox");
			wDiv.append(checkBox);
			
			div = $("<div class='flexHorizCenter'>");
			var info = $("<div class='infoImage'>");
			info.attr("data-gpio", gpioNum );
			$(info).on("click", function(event){
				var selectedGpio = event.target.dataset.gpio;
				console.log("Clicked!: " + selectedGpio);
				showGPIODialog(selectedGpio);
				//debugger;
			});
			div.append(info);
			wDiv.append(div);
			td.append(wDiv);

			tr.append(td);
		}
		table.append(tr);
	} // End of loop for GPIO items.

	$("#systemRefreshButton").button().click(
		function() {
			getData("/ESP32/SYSTEM", function(data) {
				$("#systemJsonText").val(JSON.stringify(data, null, "  "));
				// Delete all the partition rows
				$("#freeHeapSystemGeneralTab").text(data.freeHeap);
				$("#timeSystemGeneralTab").text(data.time);	
				$("#taskCountFreeRTOSTab").text(data.taskCount);
				var table = $("#partitionTable");
				table.find("> tr").remove();
				// debugger;
				data.partitions.sort(function(a, b) {
					return a.address - b.address
				});
				for (var i = 0; i < data.partitions.length; i++) {
					var row = $("<tr>");
					row.append("<td>" + typeToString(data.partitions[i].type)
							+ "</td>");
					row.append("<td>" + subTypeToString(data.partitions[i].subType) + "</td>");
					row.append("<td>" + data.partitions[i].size + "</td>");
					row.append("<td>0x" + data.partitions[i].address.toString(16)
							+ "</td>");
					row.append("<td>"
							+ (data.partitions[i].encrypted ? "Yes" : "No")
							+ "</td>");
					row.append("<td>" + data.partitions[i].label + "</td>");
					table.append(row);
				}
			})
		}
	);

	$("#wifiRefreshButton").button().click(function() {
		getData("/ESP32/WIFI", function(data) {
			$("#wifiJsonText").val(JSON.stringify(data, null, "  "));
			$("#modeWifi").text(data.mode);
			$("#staMacWifi").text(data.staMac);
			$("#apMacWifi").text(data.apMac);
			$("#staSSIDWifi").text(data.staSSID);
			$("#apSSIDWifi").text(data.apSSID);
			$("#apIpWifi").text(data.apIpInfo.ip);
			$("#apGwWifi").text(data.apIpInfo.gw);
			$("#apNetmaskWifi").text(data.apIpInfo.netmask);
			$("#staIpWifi").text(data.staIpInfo.ip);
			$("#staGwWifi").text(data.staIpInfo.gw);
			$("#staNetmaskWifi").text(data.staIpInfo.netmask);			
		})
	});

	$("#gpioRefreshButton").button().click(function() {
		getData("/ESP32/GPIO", function(data) {
			$("#gpioJsonText").val(JSON.stringify(data, null, "  "));
			// Loop through each of the GPIO items.
			for (var i=0; i<40; i++) {
				$("#gpio" + i + "Checkbox" ).attr("disabled", !data.enable[i]);
				if (data.enable[i] == true) {
					$("#gpio" + i + "Checkbox" ).attr("checked", data.out[i]);
				} else {
					$("#gpio" + i + "Checkbox" ).attr("checked", data.in[i]);
				}
				$("#gpio" + i + "Icon").toggleClass("inputImage", !data.enable[i]);
				$("#gpio" + i + "Icon").toggleClass("outputImage", data.enable[i]);
			}
		});
	});

	$("#i2sRefreshButton").button().click(function() {
		getData("/ESP32/I2S", function(data) {
			$("#i2sJsonText").val(JSON.stringify(data, null, "  "));
		})
	});
	
	$("#fileSystemRefreshButton").button().click(function() {
		var path = $("#rootPathText").val().trim();
		if (path.length == 0) {
			return;
		}
		console.log("Get the directory at: " + path);
		getData("/ESP32/FILE" + path, function(data) {
			debugger;
		});
	});
	
	$("#gpioDialog").dialog({
		autoOpen: false,
		modal: true,
		buttons: [
			{
				text: "Close",
				click: function() {
					$("#gpioDialog").dialog("close");
				}
			}
		]
	});
});
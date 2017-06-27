
/*
 * Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 * Copyright (C) 2005-2017, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Seven Du <dujinfang@x-y-t.cn>
 *
 * jquery.verto_es6_wrapper.js - Main interface
 *
 */

(function($) {
	var sources = [];

	var generateGUID = (typeof(window.crypto) !== 'undefined' && typeof(window.crypto.getRandomValues) !== 'undefined') ?
	function() {
		// If we have a cryptographically secure PRNG, use that
		// http://stackoverflow.com/questions/6906916/collisions-when-generating-uuids-in-javascript
		var buf = new Uint16Array(8);
		window.crypto.getRandomValues(buf);
		var S4 = function(num) {
			var ret = num.toString(16);
			while (ret.length < 4) {
				ret = "0" + ret;
			}
			return ret;
		};
		return (S4(buf[0]) + S4(buf[1]) + "-" + S4(buf[2]) + "-" + S4(buf[3]) + "-" + S4(buf[4]) + "-" + S4(buf[5]) + S4(buf[6]) + S4(buf[7]));
	} : function() {
		// Otherwise, just use Math.random
		// http://stackoverflow.com/questions/105034/how-to-create-a-guid-uuid-in-javascript/2117523#2117523
		return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
			var r = Math.random() * 16 | 0,
			v = c == 'x' ? r : (r & 0x3 | 0x8);
			return v.toString(16);
		});
	};

	$.verto = Verto;
	$.verto.conf = VertoConfMan;
	$.verto.liveArray = VertoLiveArray;

	var del_array = function(array, name) {
		var r = [];
		var len = array.length;

		for (var i = 0; i < len; i++) {
			if (array[i] != name) {
				r.push(array[i]);
			}
		}

		return r;
	};

	$.verto.liveTable = function(verto, context, name, jq, config) {
		var dt;
		var la = new VertoLiveArray(verto, context, name, {
			subParams: config.subParams,
			onChange: config.onChange
		});
		var lt = this;

		lt.liveArray = la;
		lt.dataTable = dt;
		lt.verto = verto;

		lt.destroy = function() {
			if (dt) {
				dt.fnDestroy();
			}
			if (la) {
				la.destroy();
			}

			dt = null;
			la = null;
		};

		la.onErr = function(obj, args) {
			console.error("Error: ", obj, args);
		};

		/* back compat so jsonstatus can always be enabled */
		function genRow(data) {
			if (typeof(data[4]) === "string" && data[4].indexOf("{") > -1) {
			var tmp = $.parseJSON(data[4]);
			data[4] = tmp.oldStatus;
			data[5] = null;
			}
			return data;
		}

		function genArray(obj) {
			var data = obj.asArray();

				for (var i in data) {
			data[i] = genRow(data[i]);
			}

			return data;
		}

		la.onChange = function(obj, args) {
			var la = this;
			var index = 0;
			var iserr = 0;

			if (!dt) {
				if (!config.aoColumns) {
					if (args.action != "init") {
						return;
					}

					config.aoColumns = [];

					for (var i in args.data) {
						config.aoColumns.push({
							"sTitle": args.data[i]
						});
					}
				}

				dt = jq.dataTable(config);
			}

			if (dt && (args.action == "del" || args.action == "modify")) {
				index = args.index;

				if (index === undefined && args.key) {
					index = la.indexOf(args.key);
				}

				if (index === undefined) {
					console.error("INVALID PACKET Missing INDEX\n", args);
					return;
				}
			}

			if (config.onChange) {
				config.onChange(obj, args);
			}

			try {
				switch (args.action) {
				case "bootObj":
					if (!args.data) {
						console.error("missing data");
						return;
					}
					dt.fnClearTable();
					dt.fnAddData(genArray(obj));
					dt.fnAdjustColumnSizing();
					break;
				case "add":
					if (!args.data) {
						console.error("missing data");
						return;
					}
					if (args.redraw > -1) {
						// specific position, more costly
						dt.fnClearTable();
						dt.fnAddData(genArray(obj));
					} else {
						dt.fnAddData(genRow(args.data));
					}
					dt.fnAdjustColumnSizing();
					break;
				case "modify":
					if (!args.data) {
						return;
					}
					// console.debug(args, index);
					dt.fnUpdate(genRow(args.data), index);
					dt.fnAdjustColumnSizing();
					break;
				case "del":
					dt.fnDeleteRow(index);
					dt.fnAdjustColumnSizing();
					break;
				case "clear":
					dt.fnClearTable();
					break;
				case "reorder":
					// specific position, more costly
					dt.fnClearTable();
					dt.fnAddData(genArray(obj));
					break;
				case "hide":
					jq.hide();
					break;

				case "show":
					jq.show();
					break;

				}
			} catch(err) {
			    console.error("ERROR: " + err);
			    iserr++;
			}

			if (iserr) {
				obj.errs++;
				if (obj.errs < 3) {
					obj.bootstrap(obj.user_obj);
				}
			} else {
				obj.errs = 0;
			}
		};

		la.onChange(la, {action: "init"});
	};

	var CONFMAN_SERNO = 1;

	$.verto.modfuncs = {};

	$.verto.confMan = function(verto, params) {
		var confMan = this;

		confMan.params = $.extend({
			tableID: null,
			statusID: null,
			mainModID: null,
			dialog: null,
			hasVid: false,
			laData: null,
			onBroadcast: null,
			onLaChange: null,
			onLaRow: null
		}, params);

		confMan.verto = verto;
		confMan.serno = CONFMAN_SERNO++;
		confMan.canvasCount = confMan.params.laData.canvasCount;

		function genMainMod(jq) {
			var play_id = "play_" + confMan.serno;
			var stop_id = "stop_" + confMan.serno;
			var recording_id = "recording_" + confMan.serno;
			var snapshot_id = "snapshot_" + confMan.serno;
			var rec_stop_id = "recording_stop" + confMan.serno;
			var div_id = "confman_" + confMan.serno;

			var html =  "<div id='" + div_id + "'><br>" +
				"<button class='ctlbtn' id='" + play_id + "'>Play</button>" +
				"<button class='ctlbtn' id='" + stop_id + "'>Stop</button>" +
				"<button class='ctlbtn' id='" + recording_id + "'>Record</button>" +
				"<button class='ctlbtn' id='" + rec_stop_id + "'>Record Stop</button>" +
				(confMan.params.hasVid ? "<button class='ctlbtn' id='" + snapshot_id + "'>PNG Snapshot</button>" : "") +
				"<br><br></div>";

			jq.html(html);

			$.verto.modfuncs.change_video_layout = function(id, canvas_id) {
				var val = $("#" + id + " option:selected").text();
				if (val !== "none") {
							confMan.modCommand("vid-layout", null, [val, canvas_id]);
				}
			};

			if (confMan.params.hasVid) {
				for (var j = 0; j < confMan.canvasCount; j++) {
					var vlayout_id = "confman_vid_layout_" + j + "_" + confMan.serno;
					var vlselect_id = "confman_vl_select_" + j + "_" + confMan.serno;


					var vlhtml =  "<div id='" + vlayout_id + "'><br>" +
						"<b>Video Layout Canvas " + (j+1) +
						"</b> <select onChange='$.verto.modfuncs.change_video_layout(\"" + vlayout_id + "\", \"" + (j+1) + "\")' id='" + vlselect_id + "'></select> " +
						"<br><br></div>";
					jq.append(vlhtml);
				}

				$("#" + snapshot_id).click(function() {
					var file = prompt("Please enter file name", "");
					if (file) {
						confMan.modCommand("vid-write-png", null, file);
					}
				});
			}

			$("#" + play_id).click(function() {
				var file = prompt("Please enter file name", "");
				if (file) {
					confMan.modCommand("play", null, file);
				}
			});

			$("#" + stop_id).click(function() {
				confMan.modCommand("stop", null, "all");
			});

			$("#" + recording_id).click(function() {
				var file = prompt("Please enter file name", "");
				if (file) {
					confMan.modCommand("recording", null, ["start", file]);
				}
			});

			$("#" + rec_stop_id).click(function() {
				confMan.modCommand("recording", null, ["stop", "all"]);
			});
		}

		function genControls(jq, rowid) {
			var x = parseInt(rowid);
			var kick_id = "kick_" + x;
			var canvas_in_next_id = "canvas_in_next_" + x;
			var canvas_in_prev_id = "canvas_in_prev_" + x;
			var canvas_out_next_id = "canvas_out_next_" + x;
			var canvas_out_prev_id = "canvas_out_prev_" + x;

			var canvas_in_set_id = "canvas_in_set_" + x;
			var canvas_out_set_id = "canvas_out_set_" + x;

			var layer_set_id = "layer_set_" + x;
			var layer_next_id = "layer_next_" + x;
			var layer_prev_id = "layer_prev_" + x;

			var tmute_id = "tmute_" + x;
			var tvmute_id = "tvmute_" + x;
			var vbanner_id = "vbanner_" + x;
			var tvpresenter_id = "tvpresenter_" + x;
			var tvfloor_id = "tvfloor_" + x;
			var box_id = "box_" + x;
			var gainup_id = "gain_in_up" + x;
			var gaindn_id = "gain_in_dn" + x;
			var volup_id = "vol_in_up" + x;
			var voldn_id = "vol_in_dn" + x;
			var transfer_id = "transfer" + x;

			var html = "<div id='" + box_id + "'>";

			html += "<b>General Controls</b><hr noshade>";

			html += "<button class='ctlbtn' id='" + kick_id + "'>Kick</button>" +
				"<button class='ctlbtn' id='" + tmute_id + "'>Mute</button>" +
				"<button class='ctlbtn' id='" + gainup_id + "'>Gain -</button>" +
				"<button class='ctlbtn' id='" + gaindn_id + "'>Gain +</button>" +
				"<button class='ctlbtn' id='" + voldn_id + "'>Vol -</button>" +
				"<button class='ctlbtn' id='" + volup_id + "'>Vol +</button>" +
				"<button class='ctlbtn' id='" + transfer_id + "'>Transfer</button>";

		if (confMan.params.hasVid) {
			html += "<br><br><b>Video Controls</b><hr noshade>";

			html += "<button class='ctlbtn' id='" + tvmute_id + "'>VMute</button>" +
				"<button class='ctlbtn' id='" + tvpresenter_id + "'>Presenter</button>" +
				"<button class='ctlbtn' id='" + tvfloor_id + "'>Vid Floor</button>" +
				"<button class='ctlbtn' id='" + vbanner_id + "'>Banner</button>";

			if (confMan.canvasCount > 1) {
						html += "<br><br><b>Canvas Controls</b><hr noshade>" +
				"<button class='ctlbtn' id='" + canvas_in_set_id + "'>Set Input Canvas</button>" +
				"<button class='ctlbtn' id='" + canvas_in_prev_id + "'>Prev Input Canvas</button>" +
				"<button class='ctlbtn' id='" + canvas_in_next_id + "'>Next Input Canvas</button>" +

				"<br>" +

				"<button class='ctlbtn' id='" + canvas_out_set_id + "'>Set Watching Canvas</button>" +
				"<button class='ctlbtn' id='" + canvas_out_prev_id + "'>Prev Watching Canvas</button>" +
				"<button class='ctlbtn' id='" + canvas_out_next_id + "'>Next Watching Canvas</button>";
			}

			html += "<br>" +
				"<button class='ctlbtn' id='" + layer_set_id + "'>Set Layer</button>" +
					"<button class='ctlbtn' id='" + layer_prev_id + "'>Prev Layer</button>" +
					"<button class='ctlbtn' id='" + layer_next_id + "'>Next Layer</button>" +
					"</div>";
			}

			jq.html(html);

			if (!jq.data("mouse")) {
				$("#" + box_id).hide();
			}

			jq.mouseover(function(e) {
				jq.data({"mouse": true});
				$("#" + box_id).show();
			});

			jq.mouseout(function(e) {
				jq.data({"mouse": false});
				$("#" + box_id).hide();
			});

			$("#" + transfer_id).click(function() {
				var xten = prompt("Enter Extension");
				if (xten) {
					confMan.modCommand("transfer", x, xten);
				}
			});

			$("#" + kick_id).click(function() {
				confMan.modCommand("kick", x);
			});

			$("#" + layer_set_id).click(function() {
				var cid = prompt("Please enter layer ID", "");
				if (cid) {
					confMan.modCommand("vid-layer", x, cid);
				}
			});

			$("#" + layer_next_id).click(function() {
				confMan.modCommand("vid-layer", x, "next");
			});
			$("#" + layer_prev_id).click(function() {
				confMan.modCommand("vid-layer", x, "prev");
			});

			$("#" + canvas_in_set_id).click(function() {
				var cid = prompt("Please enter canvas ID", "");
				if (cid) {
					confMan.modCommand("vid-canvas", x, cid);
				}
			});

			$("#" + canvas_out_set_id).click(function() {
				var cid = prompt("Please enter canvas ID", "");
				if (cid) {
					confMan.modCommand("vid-watching-canvas", x, cid);
				}
			});

			$("#" + canvas_in_next_id).click(function() {
				confMan.modCommand("vid-canvas", x, "next");
			});

			$("#" + canvas_in_prev_id).click(function() {
				confMan.modCommand("vid-canvas", x, "prev");
			});

			$("#" + canvas_out_next_id).click(function() {
				confMan.modCommand("vid-watching-canvas", x, "next");
			});

			$("#" + canvas_out_prev_id).click(function() {
				confMan.modCommand("vid-watching-canvas", x, "prev");
			});

			$("#" + tmute_id).click(function() {
				confMan.modCommand("tmute", x);
			});

			if (confMan.params.hasVid) {
				$("#" + tvmute_id).click(function() {
					confMan.modCommand("tvmute", x);
				});

				$("#" + tvpresenter_id).click(function() {
					confMan.modCommand("vid-res-id", x, "presenter");
				});
				$("#" + tvfloor_id).click(function() {
					confMan.modCommand("vid-floor", x, "force");
				});
				$("#" + vbanner_id).click(function() {
					var text = prompt("Please enter text", "");
					if (text) {
						confMan.modCommand("vid-banner", x, escape(text));
					}
				});
			}

			$("#" + gainup_id).click(function() {
				confMan.modCommand("volume_in", x, "up");
			});

			$("#" + gaindn_id).click(function() {
				confMan.modCommand("volume_in", x, "down");
			});

			$("#" + volup_id).click(function() {
				confMan.modCommand("volume_out", x, "up");
			});

			$("#" + voldn_id).click(function() {
				confMan.modCommand("volume_out", x, "down");
			});

			return html;
		}

		var atitle = "";
		var awidth = 0;

		//$(".jsDataTable").width(confMan.params.hasVid ? "900px" : "800px");

		verto.subscribe(confMan.params.laData.infoChannel, {
			handler: function(v, e) {
				if (typeof(confMan.params.infoCallback) === "function") {
					confMan.params.infoCallback(v,e);
				}
			}
		});

		verto.subscribe(confMan.params.laData.chatChannel, {
			handler: function(v, e) {
				if (typeof(confMan.params.chatCallback) === "function") {
					confMan.params.chatCallback(v,e);
				}
			}
		});

		if (confMan.params.laData.role === "moderator") {
			atitle = "Action";
			awidth = 600;

			if (confMan.params.mainModID) {
				genMainMod($(confMan.params.mainModID));
				$(confMan.params.displayID).html("Moderator Controls Ready<br><br>");
			} else {
				$(confMan.params.mainModID).html("");
			}

			verto.subscribe(confMan.params.laData.modChannel, {
				handler: function(v, e) {
					//console.error("MODDATA:", e.data);
					if (confMan.params.onBroadcast) {
						confMan.params.onBroadcast(verto, confMan, e.data);
					}

				if (e.data["conf-command"] === "list-videoLayouts") {
					for (var j = 0; j < confMan.canvasCount; j++) {
						var vlselect_id = "#confman_vl_select_" + j + "_" + confMan.serno;
						var vlayout_id = "#confman_vid_layout_" + j + "_" + confMan.serno;

						var x = 0;
						var options;

						$(vlselect_id).selectmenu({});
						$(vlselect_id).selectmenu("enable");
						$(vlselect_id).empty();

						$(vlselect_id).append(new Option("Choose a Layout", "none"));

						if (e.data.responseData) {
							var rdata = [];

							for (var i in e.data.responseData) {
								rdata.push(e.data.responseData[i].name);
							}

							options = rdata.sort(function(a, b) {
								var ga = a.substring(0, 6) == "group:" ? true : false;
								var gb = b.substring(0, 6) == "group:" ? true : false;

								if ((ga || gb) && ga != gb) {
								return ga ? -1 : 1;
								}

								return ( ( a == b ) ? 0 : ( ( a > b ) ? 1 : -1 ) );
							});

							for (var i in options) {
								$(vlselect_id).append(new Option(options[i], options[i]));
								x++;
							}
						}

						if (x) {
							$(vlselect_id).selectmenu('refresh', true);
						} else {
							$(vlayout_id).hide();
						}
					}
				} else {

					if (!confMan.destroyed && confMan.params.displayID) {
						$(confMan.params.displayID).html(e.data.response + "<br><br>");
						if (confMan.lastTimeout) {
							clearTimeout(confMan.lastTimeout);
							confMan.lastTimeout = 0;
										}
										confMan.lastTimeout = setTimeout(function() { $(confMan.params.displayID).html(confMan.destroyed ? "" : "Moderator Controls Ready<br><br>");}, 4000);
						}
					}
				}
			});

			if (confMan.params.hasVid) {
				confMan.modCommand("list-videoLayouts", null, null);
			}
		}

		var row_callback = null;

		if (confMan.params.laData.role === "moderator") {
			row_callback = function(nRow, aData, iDisplayIndex, iDisplayIndexFull) {
				if (!aData[5]) {
					var $row = $('td:eq(5)', nRow);
					genControls($row, aData);

					if (confMan.params.onLaRow) {
						confMan.params.onLaRow(verto, confMan, $row, aData);
					}
				}
			};
		}

		confMan.lt = new $.verto.liveTable(verto, confMan.params.laData.laChannel, confMan.params.laData.laName, $(confMan.params.tableID), {
			subParams: {
				callID: confMan.params.dialog ? confMan.params.dialog.callID : null
			},

			"onChange": function(obj, args) {
				$(confMan.params.statusID).text("Conference Members: " + " (" + obj.arrayLen() + " Total)");
				if (confMan.params.onLaChange) {
					confMan.params.onLaChange(verto, confMan, $.verto.enum.confEvent.laChange, obj, args);
				}
			},

			"aaData": [],
			"aoColumns": [
				{
					"sTitle": "ID",
					"sWidth": "50"
				},
				{
					"sTitle": "Number",
			"sWidth": "250"
				},
				{
					"sTitle": "Name",
			"sWidth": "250"
				},
				{
					"sTitle": "Codec",
					"sWidth": "100"
				},
				{
					"sTitle": "Status",
					"sWidth": confMan.params.hasVid ? "200px" : "150px"
				},
				{
					"sTitle": atitle,
					"sWidth": awidth,
				}
			],
			"bAutoWidth": true,
			"bDestroy": true,
			"bSort": false,
			"bInfo": false,
			"bFilter": false,
			"bLengthChange": false,
			"bPaginate": false,
			"iDisplayLength": 1400,

			"oLanguage": {
				"sEmptyTable": "The Conference is Empty....."
			},

			"fnRowCallback": row_callback
		});
	};

	$.verto.confMan.prototype.modCommand = function(cmd, id, value) {
		var confMan = this;

		confMan.verto.call("verto.broadcast", {
			"eventChannel": confMan.params.laData.modChannel,
			"data": {
				"application": "conf-control",
				"command": cmd,
				"id": id,
				"value": value
			}
		});
	};

	$.verto.confMan.prototype.sendChat = function(message, type) {
		var confMan = this;
		confMan.verto.call("verto.broadcast", {
			"eventChannel": confMan.params.laData.chatChannel,
			"data": {
				"action": "send",
				"message": message,
				"type": type
			}
		});
	};

	$.verto.confMan.prototype.destroy = function() {
		var confMan = this;

		confMan.destroyed = true;

		if (confMan.lt) {
			confMan.lt.destroy();
		}

		if (confMan.params.laData.chatChannel) {
			confMan.verto.unsubscribe(confMan.params.laData.chatChannel);
		}

		if (confMan.params.laData.modChannel) {
			confMan.verto.unsubscribe(confMan.params.laData.modChannel);
		}

		if (confMan.params.mainModID) {
			$(confMan.params.mainModID).html("");
		}
	};

	$(window).bind('beforeunload', function() {
		for (var f in $.verto.unloadJobs) {
			$.verto.unloadJobs[f]();
		}

		if ($.verto.haltClosure) return $.verto.haltClosure();

		for (var i in $.verto.saved) {
			var verto = $.verto.saved[i];
			if (verto) {
				verto.purge();
				verto.logout();
			}
		}

		return $.verto.warnOnUnload;
	});

	$.FSRTC = VertoRTC;

})(jQuery);

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */

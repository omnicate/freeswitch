(function() {
	'use strict';

	angular
		.module('vertoControllers')
		.controller('StatsController', ['$scope', 'verto',
				function($scope, verto) {
					console.debug("stats window");
					$scope.peer = verto.data.call.rtc.peer;
					$scope.videoBytes = [
						[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,],
						[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,]
					];
					$scope.videoLabels = ["","","","","","","","","","","","","","","","","","","","","","","","","","","","","",""];
					$scope.videoSeries=["Recieved", "Sent"];
					$scope.statsTimeout = 0;
					$scope.statsEnable = true;
					$scope.lastVidSend = 0;
					$scope.lastVidRecv = 0;
					$scope.lastAudioSend = 0;
					$scope.lastAudioRecv = 0;

					getStats($scope.peer.peer);


					function clickit(points, evt) {
						$scope.statsEnable ^= 1;
						console.debug("$scope.statsEnable:" + $scope.statsEnable);
						if ( !$scope.statsEnable ){
							clearTimeout($scope.statsTimeout);
						}
					};

					function getStats(pc) {
						myGetStats(pc, function (results) {
							var lastPeriod = 0;
							
							//shit goes here to handle pushing stats into graph
							results.forEach(function (a,b,c) {
								// if (a.transportId == "Channel-audio-1" && a.bytesSent > 0) {
								if (a.transportId == "Channel-video-1"){
									//console.warn("video: ");
									//console.warn(a);
									if (a.bytesSent > 0){
										console.warn("bytes [" + a.bytes + "] last bytes ["+ $scope.lastVidSend + "]");
										if (a.bytesSent > $scope.lastVidSend) {
											lastPeriod = a.bytesSent - $scope.lastVidSend;
											console.warn("lastPeriod after math ["+lastPeriod+"]");
										} else {
											lastPeriod = a.bytesSent;
											console.warn("lastPeriod No math ["+lastPeriod+"]");
										}
										$scope.lastVidSend = a.bytesSent;

										var junk = $scope.videoBytes[1].shift();
										$scope.videoBytes[1].push(lastPeriod);
										console.warn(" after push $scope.videoBytes[1] :");
										console.warn($scope.videoBytes[1]);
									}
									if (a.bytesReceived > 0) {
										console.warn("bytes [" + a.bytes + "] last bytes ["+ $scope.lastVidRecv + "]");
										if (a.bytesReceived > $scope.lastVidRecv) {
											lastPeriod = a.bytesReceived - $scope.lastVidRecv;
											console.warn("lastPeriod after math ["+lastPeriod+"]");
										} else {
											lastPeriod = a.bytesReceived;
											console.warn("lastPeriod No math ["+lastPeriod+"]");
										}
										$scope.lastVidRecv = a.bytesReceived;

										var junk = $scope.videoBytes[0].shift();
										$scope.videoBytes[0].push(lastPeriod);
										console.warn(" after push $scope.videoBytes[0] :");
										console.warn($scope.videoBytes[0]);
									}
								}
								if (a.transportId == "Channel-audio-1"){
									console.warn("audio: ");
									console.warn(a);
									if (a.bytesSent > 0){
										// do sent here

									}
									if (a.bytesReceived > 0) {
										// received goes here
										
									}
								}
								// transportId: "Channel-video-1" && bytesReceived: "262921"

							});

							// $scope.videoBytes.push(results);
							$scope.statsTimeout = setTimeout(function () {
								getStats(pc);
							}, 1000);
							
						});
					}

					function myGetStats(pc, callback) {
						if (!!navigator.mozGetUserMedia) {
							pc.getStats(
									function (res) {
										var items = [];
										res.forEach(function (result) {
											items.push(result);
										});
										callback(items);
									},
									callback
									);
						} else {
							pc.getStats(function (res) {
								var items = [];
								res.result().forEach(function (result) {
									var item = {};
									result.names().forEach(function (name) {
										item[name] = result.stat(name);
									});
									item.id = result.id;
									item.type = result.type;
									item.timestamp = result.timestamp;
									items.push(item);
								});
								callback(items);
							});
						}
					};
					$scope.onClick = clickit;

					$scope.$on(
							"destroy",
							function(event){
								//$timeout.cancel($scope.statsTimeout);
								clearTimeout($scope.statsTimeout);
							}
					);

				}
	]);
})();

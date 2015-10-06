(function() {
	'use strict';

	angular
		.module('vertoControllers')
		.controller('StatsController', ['$scope', 'verto',
				function($scope, verto) {
					console.debug("stats window");
					$scope.statsEnable = false;
					$scope.peer = verto.data.call.rtc.peer;
					$scope.videoBytes = [[0,0,0,0,0,0]];
					$scope.videoLabels = [0,0,0,0,0,0];
					$scope.series=["Video"];
					$scope.statsTimeout = 0;
					getStats($scope.peer.peer);

					function clickit(points, evt) {
						$scope.statsEnable ^= 1;
						if ( $scope.statsEnable ){
							getStats($scope.peer.peer);
						} else {
							clearTimeout($scope.statsTimeout);
						}
					};

					function getStats(pc) {
						myGetStats(pc, function (results) {
							
							if ($scope.videoBytes.length >= 180) {
								$scope.videoBytes.shift();
							}
							console.warn(results);
							
							//shit goes here to handle pushing stats into graph

							// $scope.videoBytes.push(results);
							
							if ( $scope.statsEnable ){
								$scope.statsTimeout = setTimeout(function () {
									getStats(pc);
								}, 1000);
							}

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
				}
	]);
})();

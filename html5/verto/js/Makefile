JSFILES=src/jquery.FSRTC.js src/jquery.jsonrpcclient.js src/jquery.verto.js src/vendor/adapter-latest.js
ES6FILES=verto-es6.js src/jquery.verto.es6_wrapper.js src/vendor/adapter-latest.js

all: jsmin verto-min.js

jsmin: jsmin.c
	$(CC) $< -o $@

verto-min.js: jsmin $(JSFILES)
	cat $(JSFILES) | ./jsmin > $@

verto-max.js: jsmin $(JSFILES)
	cat $(JSFILES) > $@

clean:
	rm -f verto-min.js verto-max.js jsmin *~

install-demo: all
	cp verto-min.js ../demo/js

install-maxdemo: all verto-max.js
	cp verto-max.js ../demo/js/verto-min.js

install-video_demo: all
	cp verto-min.js ../video_demo/js
	cp verto-min.js ../video_demo-live_canvas/js

install-video_maxdemo: all verto-max.js
	cp verto-max.js ../video_demo/js/verto-min.js
	cp verto-max.js ../video_demo-live_canvas/js/verto-min.js

verto-es6.js:
	# babel src/es6 > $@
	npm run build
	cat src/vendor/*.js >> $@
	cat $(ES6FILES) | ./jsmin > ../demo/js/verto-min.js
	cat $(ES6FILES) | ./jsmin > ../video_demo/js/verto-min.js
	cat $(ES6FILES) | ./jsmin > ../video_demo-live_canvas/js/verto-min.js
	rm verto-es6.js

es6: verto-es6.js
	echo "OK"

es6clean:
	rm -f verto-es6.js

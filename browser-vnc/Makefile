build:
	docker build -t rcarmo/browser-vnc .

run: # remember that chrome will only run with --no-sandbox
	docker run -d -p 2222:22 rcarmo/browser-vnc 
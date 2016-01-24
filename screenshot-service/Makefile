export PORT=8000
default:
	docker build -t rcarmo/screenshot-service .

run:
	docker run -d -p 8000:8000 -e API_KEY=password rcarmo/screenshot-service

test:
	$(eval GUID=`curl --data "key=password&url=http://google.com" http://localhost:$(PORT)/; sleep 3`)
	wget -v http://localhost:$(PORT)/$(GUID)

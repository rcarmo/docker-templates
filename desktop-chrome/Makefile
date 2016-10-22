export PORT=8000
export IMAGE_NAME=rcarmo/desktop-chrome
default:
	docker build -t $(IMAGE_NAME):latest -f Dockerfile .

tiger:
	docker build -t $(IMAGE_NAME):tiger -f Dockerfile_tiger .

run:
	docker run -d -p 2211:22 $(IMAGE_NAME)

rmi:
	docker rmi -f $(IMAGE_NAME)

push:
	-docker push $(IMAGE_NAME)
	-docker push $(IMAGE_NAME):tiger

clean:
	-docker rm -v $$(docker ps -a -q -f status=exited)
	-docker rmi $$(docker images -q -f dangling=true)
	-docker rmi $(IMAGE_NAME)

export IMAGE_NAME=rcarmo/azure-sandbox
default:
	docker build -t $(IMAGE_NAME) .

run:
	docker run -d -p 5900:5901 $(IMAGE_NAME) /quickstart.sh

rmi:
	docker rmi -f $(IMAGE_NAME)

push:
	docker push $(IMAGE_NAME)

clean:
	-docker rm -v $$(docker ps -a -q -f status=exited)
	-docker rmi $$(docker images -q -f dangling=true)
	-docker rmi $(IMAGE_NAME)

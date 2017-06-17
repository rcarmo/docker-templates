export IMAGE_NAME=rcarmo/screenshot-chrome
export VCS_REF=`git rev-parse --short HEAD`
export BUILD_DATE=`date -u +"%Y-%m-%dT%H:%M:%SZ"`
base:
	docker build --build-arg VCS_REF=$(VCS_REF) \
		     --build-arg BUILD_DATE=$(BUILD_DATE) \
		     -t $(IMAGE_NAME) -f Dockerfile .

run:
	docker run -d -p 5900:5901 $(IMAGE_NAME) /quickstart.sh

rmi:
	-docker rmi -f $(IMAGE_NAME)

push:
	-docker push $(IMAGE_NAME)

clean:
	-docker rm -v $$(docker ps -a -q -f status=exited)
	-docker rmi $$(docker images -q -f dangling=true)

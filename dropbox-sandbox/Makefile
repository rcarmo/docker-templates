export IMAGE_NAME=rcarmo/dropbox
export VCS_REF=`git rev-parse --short HEAD`
export VCS_URL=https://github.com/rcarmo/docker-templates
export BUILD_DATE=`date -u +"%Y-%m-%dT%H:%M:%SZ"`

build:
	docker build --build-arg BUILD_DATE=$(BUILD_DATE) \
		--build-arg VCS_REF=$(VCS_REF) \
		--build-arg VCS_URL=$(VCS_URL) \
		-t $(IMAGE_NAME) .

clean:
	-docker rm -v $$(docker ps -a -q -f status=exited)
	-docker rmi $$(docker images -q -f dangling=true)
	-docker rmi $$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '$(IMAGE_NAME)')

push:
	docker push $(IMAGE_NAME)

test:
	docker run \
		--name=dropbox \
		-v /tmp/data:/Dropbox \
		--net=host \
		$(IMAGE_NAME)

default:
	docker build -t rcarmo/ink-builder .

ink:
	cd $(HOME)/Development
	git pull git@github.com:rcarmo/Ink.git
	cd $(HOME)/Development/Ink
	echo "Type 'cd /Ink; npm install' to fetch dependencies, then 'grunt' to build"
	docker run -v $(pwd):/Ink -ti rcarmo/ink-builder

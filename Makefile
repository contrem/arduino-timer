NAME := arduino-timer
VERSION := $(shell grep version library.properties | cut -d= -f2)

$(NAME)-$(VERSION).zip:
	git archive HEAD --prefix=$(@:.zip=)/ --format=zip -o $@

tag:
	git tag $(VERSION)

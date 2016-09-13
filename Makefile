.PHONY: all
all: windowpane cowpatty2windowpane

windowpane: windowpane.cpp
	$(CXX) windowpane.cpp -o windowpane -std=c++11 -lcrypto++

cowpatty2windowpane: cowpatty2windowpane.cpp
	$(CXX) cowpatty2windowpane.cpp -o cowpatty2windowpane -std=c++11

.PHONY: clean
clean:
	$(RM) windowpane
	$(RM) cowpatty2windowpane

.PHONY: install
install:
	echo install $(DESTDIR)/usr/bin/windowpane
	echo install $(DESTDIR)/usr/bin/cowpatty2windowpane

.PHONY: uninstall
uninstall:
	echo $(RM) $(DESTDIR)/usr/bin/windowpane
	echo $(RM) $(DESTDIR)/usr/bin/cowpatty2windowpane

.PHONY: all
all: windowpane cowpatty2windowpane pyritsort

windowpane: windowpane.cpp
	$(CXX) windowpane.cpp -o windowpane -std=c++11 -lcryptopp -pthread -O2

cowpatty2windowpane: cowpatty2windowpane.cpp
	$(CXX) cowpatty2windowpane.cpp -o cowpatty2windowpane -std=c++11 -O2

pyritsort: pyritsort.cpp
	$(CXX) pyritsort.cpp -o pyritsort -std=c++11 -O2

.PHONY: clean
clean:
	$(RM) windowpane
	$(RM) cowpatty2windowpane
	$(RM) pyritsort

.PHONY: install
install:
	echo install windowpane          $(DESTDIR)/usr/bin/windowpane
	echo install cowpatty2windowpane $(DESTDIR)/usr/bin/cowpatty2windowpane
	echo install pyritsort           $(DESTDIR)/usr/bin/pyritsort

.PHONY: uninstall
uninstall:
	echo $(RM) $(DESTDIR)/usr/bin/windowpane
	echo $(RM) $(DESTDIR)/usr/bin/cowpatty2windowpane
	echo $(RM) $(DESTDIR)/usr/bin/pyritsort


WIRE_H = Wire.h
SMARTBATT_OBJS = smartbatt.opp
SMARTBATT = smartbatt

all: $(SMARTBATT)

$(SMARTBATT): $(SMARTBATT_OBJS) $(WIRE_H)
	$(CXX) $(CFLAGS) $(LIBS) -o $@ $(SMARTBATT_OBJS)

%.opp: %.cpp
	$(CXX) -c $(CFLAGS) $(LIBS) -o $@ $^

clean:
	rm -f $(SMARTBATT_OBJS) $(SMARTBATT)

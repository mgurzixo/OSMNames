BASE=/home/mgouget/dev
TEST_BASENAME=test

# TEST_COUNTRIES=paris
TEST_COUNTRIES=portugal-latest
# TEST_COUNTRIES=luxembourg-latest

WORLD_BASENAME=world


all:

extractworld: 
	time sh -x ./extractworld.sh

extracttest:
	time sh -x ./extracttest.sh $(TEST_COUNTRIES)

#include "local_handler.h"

using namespace std;

/*--------------------------------------------------
This is the constructor for the Decawave interface.
It takes in two references to an x and y position so
it can modify them without needing to return
anything. They are atomic which means they can be 
written to and read from different threads safely.
--------------------------------------------------*/
decawave_handler::decawave_handler(atomic<double> * x_ref, atomic<double> * y_ref) {
	prv_x_pos_ref     = x_ref;
	prv_y_pos_ref     = y_ref;
	//setting position to 1,1 initially for testing but in the future the plow should wait until
	//its first positional update to do anything. 
	*prv_x_pos_ref    = 1.0;
	*prv_y_pos_ref    = 1.0;
}

/*--------------------------------------------------
Main execution loop for decawave interface.
--------------------------------------------------*/
void decawave_handler::run() {

	/*--------------------------------------------------
	Local variables
	--------------------------------------------------*/
	string building_scan   = "";
	string recent_scan     = "";
	Serial * SP            = new Serial( DECAWAVE_COM_PORT );
	char incomingData[256] = "";			   									   
	int dataLength         = 255;
	int readResult         = 0;
	int poll_counter       = 0;
	bool first_scan        = true;

	/*---------------------------------------
	Run while connected to the arduino.
	---------------------------------------*/
	while (SP->IsConnected())
	{
		readResult = SP->ReadData(incomingData, dataLength);
		incomingData[readResult] = 0;

		/*---------------------------------------
		The basic idea of this is that if we have
		not received data in a while than we can
		be pretty sure the last group of data we
		received was a full scan. The decawave
		system should be slow enough in between
		readings that this poll counter will get
		to 2000 and trigger a location analysis
		on the last received data.
		---------------------------------------*/
		if (strlen(incomingData) != 0) {
			//cout << "poll counter: " << poll_counter << endl;
			poll_counter = 0;
			//cout << "got new data: " << incomingData << endl;
		}
		else {
			poll_counter++;
		}

		/*------------------------------------------------
		Ignores large string of incoming data at beginning
		------------------------------------------------*/
		if (strlen(incomingData) < 70) {

			if (poll_counter > 2000) {
				recent_scan = building_scan;

#if PRINT_RECENT_DATA_PKT
				//cout << "================== NEW DECAWAVE LOCATION PACKET ================" << endl;
				for (int i = 0; i < recent_scan.length(); i++) {
					//cout << recent_scan[i];
				}
#endif
				/*--------------------------------------------------
				reset the building scan and add any data is present
				--------------------------------------------------*/
				building_scan = "";
				building_scan += incomingData;

				/*--------------------------------------------------
				Local variables
				--------------------------------------------------*/
				bool first_open_bracket  = false;
				bool second_open_bracket = false;
				bool x_done              = false;
				bool y_done              = false;
				string x_str;
				string y_str;

				/*--------------------------------------------------
				The following goes through the data packet and
				parses the important information.
				--------------------------------------------------*/
				for (int i = 0; i < recent_scan.length(); i++) {
					if (!first_open_bracket && recent_scan[i] == '[') {
						first_open_bracket = true;
					}
					else if (first_open_bracket && recent_scan[i] == '[') {
						second_open_bracket = true;
					}
					if (first_open_bracket && second_open_bracket && !x_done) {
						x_str += recent_scan[i];
					}
					if (first_open_bracket && second_open_bracket && recent_scan[i] == ',') {
						x_done = true;
						continue;
					}
					if (first_open_bracket && second_open_bracket && x_done && !y_done) {
						y_str += recent_scan[i];
					}
					if (first_open_bracket && second_open_bracket && x_done && recent_scan[i] == ',') {
						y_done = true;
					}
				}

				/*--------------------------------------------------
				The following converts the strings to values
				--------------------------------------------------*/
				if (first_scan == false) {
					try {
						double temp_x = 0.0, temp_y = 0.0;
						int x_tot = 4;
						if (x_str.find('-') != -1) {
							x_tot++;
						}
						if (x_str.length() > 0) {
							string x_string = x_str.substr(1, x_tot);
							istringstream i(x_string);
							if (!(i >> temp_x)) {
								std::cout << "Conversion error with x-location in decawave handler.";
							}
						}

						int y_tot = 4;
						if (y_str.substr(0, 4).find('-') != -1) {
							y_tot++;
						}
						if (y_str.length() > 0) {
							string y_string = y_str.substr(0, y_tot);
							istringstream i(y_string);
							if (!(i >> temp_y)) {
								std::cout << "Conversion error with y-location in decawave handler.";
							}
						}
						/*--------------------------------------------------
						Set the actual position values to the converted ones
						--------------------------------------------------*/
						if (temp_x == 0 && temp_y == 0)
						{
							continue;
						}
						*prv_x_pos_ref = temp_x;
						*prv_y_pos_ref = temp_y;
						cout << "updated" << endl;
#if PRINT_PARSED_LOCATION
						cout << "x location: ";
						cout << temp_x;
						/*int maxScanRangeX = x_str.length() - 1;
						for (int i = 1; i < maxScanRangeX; i++) {
						cout << x_str[i];
						}
						int maxScanRangeY = y_str.length() - 4;
						if (y_str.length() > 4)
						{
						maxScanRangeY = 5;
						}
						else
						{
						maxScanRangeY = 0;
						}*/
						cout << endl << "y location: ";
						cout << temp_y;
						/*for (int i = 0; i < 5; i++) {
						cout << y_str[i];
						}*/
						cout << endl;
#endif
					}

					catch (const std::exception &exc)
					{
						/*--------------------------------------------------
						Print the error if there was one
						--------------------------------------------------*/
						std::cerr << exc.what() << endl;
					}
				}
				first_scan = false;

//#if PRINT_PARSED_LOCATION
//				cout << "x location: ";
//				cout << temp_x;
//				/*int maxScanRangeX = x_str.length() - 1;
//				for (int i = 1; i < maxScanRangeX; i++) {
//					cout << x_str[i];
//				}
//				int maxScanRangeY = y_str.length() - 4;
//				if (y_str.length() > 4)
//				{
//					maxScanRangeY = 5;
//				}
//				else
//				{
//					maxScanRangeY = 0;
//				}*/
//				cout << endl << "y location: ";
//				cout << temp_y;
//				/*for (int i = 0; i < 5; i++) {
//					cout << y_str[i];
//				}*/
//				cout << endl;
//#endif

			}
			/*--------------------------------------------------
			If it hasn't been long enough for a full data pkt
			to have been received then just add the partial 
			data to the next packet.
			--------------------------------------------------*/
			else {
				building_scan += incomingData;
			}

		}

	}
}
#include "profile_segment.h"
#include <iomanip>
#include <iostream>
#include <tuple>
#include <queue>
#include <cmath>

profile_segment::profile_segment(vector<uint8_t> d, string n, double g, double o, int as) : data(d), name(n), gain(g), offset(o), autogain_scale(as) {
	id = data[0];                             // sensorID to identify the type of sensor family
	nbyte = ((data[1] & 0x0f) << 8 )+data[2]; // number of bytes in segment
	unpk = (data[3] & 0xF0) >> 4;             // data-packing algorithm identifier
	sensor_id = data[3] & 0x0F;               // ID to identify the type of sensor family
	pro = data[4];                            // how profile is averaged and acquired
	message_index = data[5] & 0x0F;           // profile sub-segment ID
	total_scans = (data[8] <<8) + data[9];

	// unpack data
	if (unpk == 0 || unpk == 4 || unpk == 8 || unpk == 12) {
		index = message_index*300;
		Unpack_Integer((unpk>>2)+1);
	}
	if (unpk == 1) {
		index = message_index*300;
		Unpack_1p0(0); // standard 1st diff
	}
	if (unpk == 5) {
		index = message_index*300;
		Unpack_1p0(0); // standard 1st diff w/ bad scan enabled
	}
	if (unpk == 9) {
		index = message_index*300;
		Unpack_1p0(1); // 1st diff with autogain
	}
	if (unpk == 2) {
		// this sub-message uses 2nd diff compression
		index = data[6]*16;
		Unpack_2p0();
	}
	if (unpk == 6) { // BGC 2nd diff w/ bad bit capability
		// this sub-message uses 2nd diff compression
		index = (data[6] << 8) + data[7];
		Unpack_BGC_2d(0);
	}
	if (unpk == 10) { // BGC 2nd diff w/ autogain
		index = (data[6] << 8) + data[7];
		Unpack_BGC_2d(1);
	}
}

void profile_segment::Unpack_Integer(int s) {
    for( int i = 0; i < nbyte; i += s) {
        int v = 0;
        for( int b = 0; b < s; b++) {
            v += data[i+b];
            v <<= 8;
        }
        raw_counts.push_back(v);
    }
}

// Unpack data using 1st difference algorithm
void profile_segment::Unpack_1p0(bool autogain = 0) {
	int auto_gain, auto_offset, ptr;
	if (autogain) {
	  auto_offset = data[10];
	  auto_gain = data[11];
      ptr = 12;
    }
	else {
	  ptr = 10;
    }

    unsigned int scale, x0; // = scalar + 1st pt in each sub-block
    unsigned int isub = 25; //=max # points in a sub-block: last might be a partial
    int dx;

    unsigned int n = 0; // counter for bytes in data
    while ( n < nbyte -4 ) {
      scale = data[ptr]; ptr++;
      x0 = (data[ptr] << 8)+data[ptr+1]; ptr += 2;
      n += 3;
      raw_counts.push_back(x0);                  // save the value
      isub = 25;
      while ( n< nbyte-4 && --isub ) {
        // integrate the time series in the sub-block -------------
        dx = data[ptr]; ptr++;
        n++;  // 1st-diff value
        if ( dx> 0x7f )
          dx -= 0x100;  // Watch the sign!!
        x0 += dx*scale;               // integrate
        raw_counts.push_back(x0);               // and save
      } // while processing sub-block ------------------------------
      // if not at end of b[], go unpack the next sub-block
    } // end while processing the input string b[] ++++++++++++++++++++++++
  } // end Unpack_Profile_Data_1p0

void profile_segment::Unpack_BGC_2d(bool autogain) {
    int lim[6] = { 0x8,  0x80,  0x800,  0x8000,  0x80000,  0x800000 }; // largest values for each packing factor
    int ref[6] = { 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000 }; // reference value to make negative values
    std::vector <long int> D1; // first difference values
    std::vector <long int> D2; // first difference values
	std::vector <long int> buff;
	std::vector <uint8_t> badbit;
    std::queue<uint8_t> nib; // nibbles
	double val;

	int npts = ((data[1] & 0x0F) << 8) + data[2];
	int nsubs = (data[6]<<8) + data[7];
	bool badbit_flag; // indicates bad scans in current segment, badbit mask follows nibble byte
	int badbit_cnt;   // # of scans that are masked in badbit mask
	int D2_size;      // size [nibbles] of 2nd difference
	int D1_count;     // number of 1st differences; one more than 2nd differences, one less than scan count
	int tmp;
	int cnt;

	int n = 0;
	int ptr = 10;
	int again, aoffset; // auto gain/offset

	while (data[ptr] != 0x3b) {
		D1.clear();
		D2.clear();
		while ((int)nib.size() > 0) {
			//cout << "nib not empty [" << (int)nib.size() << "] " << (int)nib.front() << " pop" << endl;
			nib.pop();
		}
		badbit.clear();
		badbit_cnt = 0;
		badbit_flag = data[ptr] & 0x80;

		D2_size = ((data[ptr] & 0x70) >> 4) + 1; // # nibbles for each 2nd difference [0..5] (raw value is # nibbles - 1)
		D1_count = data[ptr++] & 0x0F;

		// test for badbit_flag
		if (badbit_flag) {
			tmp = (data[ptr] << 8) + data[ptr+1];
			std::cout << "Badbit flag: " << std::hex << std::setw(4) << std::setfill('0') << tmp << std::dec << std::endl;
			ptr += 2;
			for(int x = 0; x < 15; x++) {
				badbit.push_back( (tmp & (1<<x)) > 0 );
				if (tmp & (1<<x))
					badbit_cnt++;
			}
		}

		// If autogain is enabled
		if (autogain) {
			aoffset = data[ptr++];
			again = data[ptr++];
			//std::cout << "auto gain:" << again << " offset:" << aoffset << std::endl;
		}

		// Read first count
		cnt = (data[ptr]<<8) + data[ptr+1];
		buff.push_back(cnt);
		n++;
		ptr += 2;
		//std::cout << "1st val push back: " << cnt << std::endl;

		// Read the first difference scan (if sent)
		if (D1_count) {
			tmp = (data[ptr]<<8) + data[ptr+1];
			if (tmp >= 0x8000)
				tmp -= 0x10000;
			D1.push_back(tmp);
			ptr += 2;
			n++;
		}

		if (D1_count > 1) {
			// Read in nibbles (if sent)
			int i = D2_size * (D1_count - 1 - badbit_cnt); // D1_count - 1 = # nibbles; bad measurements are not sent, so subtract these
			while (i > 1) {
				nib.push( (data[ptr] & 0xF0) >> 4);
				nib.push( data[ptr++] & 0x0F );
				i-=2;
			}
			if (i==1)
				nib.push( (data[ptr++] & 0xF0) >> 4 );	// for odd numbers of nibbles, there could be one last one

			// Compute second differences
			for(int x = 0; x < D1_count - 1 - badbit_cnt; x++) {
				int v = 0;
				int i = 0;
				for(; i < D2_size; i++) {
					v <<= 4;
					v += nib.front();
					nib.pop();
				}
				if (v >= lim[i-1])
					v -= ref[i-1]; // convert unsigned to signed
				D2.push_back(v);
				n++;
			}

			// Compute 1st differences
			for (unsigned int n=0; n<D1_count-1; n++) { // there will be D1_count-1 2nd differences
				D1.push_back(D1[n]+D2[n]);
			}
		}

	   	// compute values
		for (unsigned int z=0; z < D1_count; z++) {
			cnt = buff.back()+D1[z];
			buff.push_back(cnt);
    	}
		if (autogain) {
			//int scalar_offset = 20000;//config["packets"][type]["auto_offset"]; // for auto-gain variables, offset value is multiplied by this constant
			if (autogain_scale) {
				//std::cout <<" * Using autogain_scale " << autogain_scale << std::endl;
			}
			if (!autogain_scale) {
				std::cout << " * WARNING - autogain_scale is not set" << std::endl;
			}
			for (auto b : buff) {
				//std::cout << "cnt: " << b << " gain: " << again << " offset:" << aoffset << std::endl;
				//std::cout << "cnt: " << b << "; 1024(1966080 + " << b << " + (" << aoffset << " x " << autogain_scale << ")) = " << 1024*(1966080+b+(aoffset*autogain_scale)) << std::endl;
				if (id == 0x24) {
					// pH uses a linear auto-gain
					raw_counts.push_back((b * again) + (aoffset*autogain_scale));
				}
				else if (id == 0x25) {
					// OCR uses an exponential auto-gain; (2 ^ again)
					raw_counts.push_back((b << again) + (aoffset*autogain_scale));
				}
				else {
					std::cout << " * warning, auto-gain only implemented for pH and OCR" << std::endl;
				}
			}
		}
		else {
			for (auto b : buff)
				raw_counts.push_back(b);
		}
		buff.clear();

		// insert fill values for flagged bad scans
		int r = 0;
		if (badbit_flag) {
			std::vector<long int> tmp;
			for( int x = 0; x < D1_count+1; x++ ) {
				if (badbit[x])
					tmp.push_back(-999);
				else
					tmp.push_back( raw_counts[r++] );
			}
			raw_counts = tmp;
		}
    }

	if (raw_counts.size()==0) {
		std::cout << " * warning, empty packet 0x3b found at byte 10" << std::endl;
	}
}


// Unpack data using 2nd difference algorithm
void profile_segment::Unpack_2p0() {
	int ptr = 7; // BG 2023/11/30

    unsigned int nnib = 0; // counter to keep track of nibbles
    unsigned int isub;
    unsigned int pmsk[8] = { 0xe00000, 0x1c0000, 0x038000, 0x007000, 0xe00, 0x1c0, 0x038, 0x007 }; // mask to extract packing factors
    unsigned int pshft[8] = { 21, 18, 15, 12, 9, 6, 3, 0 }; // shift for packing factors
    int lim[6] = { 0x8,  0x80,  0x800,  0x8000,  0x80000,  0x800000 }; // largest values for each packing factor
    int ref[6] = { 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000 }; // reference value to make negative values
    int nibmsk[2] = { 0xf0, 0x0f };
    int nibsft[2] = { 4, 0};
    unsigned int p_fact[32]; // packing factor
    std::vector <long int> D; // first difference values
    std::vector <long int> D2; // first difference values

    npts = ((data[ptr]&0x7F) << 8)+data[ptr+1]; ptr += 2;
    //npts = (data[ptr+1] << 8)+data[ptr]; ptr += 2;

    raw_counts.push_back((data[ptr] << 16)+(data[ptr+1] << 8)+data[ptr+2]); ptr += 3;
    D.push_back((data[ptr] << 16)+(data[ptr+1] << 8)+data[ptr+2]); ptr += 3;

    if (D[0] >= 0x800000)
      D[0] -= 0x1000000;

    // get packing factors
    int nf = 0;
    for (size_t n=0; n<4; ++n) {
      long int inpt = (data[ptr] << 16)+(data[ptr+1] << 8)+data[ptr+2]; ptr += 3;
      for (size_t n1=0; n1<8; ++n1) {
        p_fact[nf] = (inpt & pmsk[n1]) >> pshft[n1];
        ++nf;
      }
    }
    if (npts == 1)
      return;
    else if (npts ==2) {
      raw_counts.push_back(raw_counts[0]+D[0]);
      return;
    }

    // read in 2nd differences
    isub = npts-2;
    int m = 0; // pointer to sub-packet
    int n1 = 0;
    while ( isub > 0 ) {
      if (p_fact[m] == 0) {
        printf(" -- warning: bin %2d is empty\n",m++);
        continue;
      }
      if (m >= 32) {
        printf(" ** Curvature packed data missing %d scans\n",isub);
        npts -= isub;
        break;
      }

      int m_in = 16;
      if (isub<16)
        m_in = isub; // number of values for this sub-packet
      for (int n=0; n<m_in; n++) {
        D2.push_back(0);
        for (unsigned int n2=0; n2<p_fact[m]; n2++) {
          // get data a nibble at a time
          D2[n1] = (D2[n1] << 4)+((data[ptr] & nibmsk[nnib]) >> nibsft[nnib]);
          nnib++;
          ptr += floor(nnib/2);
          nnib = nnib % 2;
        }
        // check for negative values
        if (D2[n1] > lim[p_fact[m]-1]) {
          D2[n1] -= ref[p_fact[m]-1];
        }
        n1++;
      }
      m++;
      isub -= m_in;
    }
    // compute 1st differences
    for (unsigned int n=1; n<npts-1; n++) { // there will be npts-2 D2 values
      D.push_back(D[n-1]+D2[n-1]);
    }
    // compute values
    for (unsigned int n=1; n<npts; n++) {
      raw_counts.push_back(raw_counts[n-1]+D[n-1]);
    }
} // end Unpack_Profile_Data_2p0

// overload < operator to sort segments in this order: id,sensor_id,pro,message_index
bool profile_segment::operator<(const profile_segment &rhs) const {
	// implement sorting using tuples
	return std::tie(id,sensor_id,pro,message_index) < std::tie(rhs.id,rhs.sensor_id,rhs.pro,rhs.message_index);
}

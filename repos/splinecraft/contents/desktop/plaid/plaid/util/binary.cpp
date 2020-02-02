#include "binary.h"


using namespace plaid;



std::string plaid::ToStdString(const String &str)
{
	std::string res(str.length(), ' ');
	std::copy(str.begin(), str.end(), res.begin());
	return res;
}
String plaid::ToPGString(const std::string &str)
{
	String res(str.length(), L' ');
	std::copy(str.begin(), str.end(), res.begin());
	return res;
}


static const Uint8
	F1 = 0x80, L7 = 0x7F, B7 = 0x80,
	F2 = 0xC0, L6 = 0x3F, B6 = 0x40,
	F3 = 0xE0, L5 = 0x1F, B5 = 0x20,
	F4 = 0xF0, L4 = 0x0F, B4 = 0x10,
	F5 = 0xF8, L3 = 0x07, B3 = 0x08,
	F6 = 0xFC, L2 = 0x03, B2 = 0x04,
	F7 = 0xFE, L1 = 0x01, B1 = 0x02, B0 = 0x01;
static const Uint32
	THRESH07 = 0xFFFFFF80,
	THRESH11 = 0xFFFFF800,
	THRESH16 = 0xFFF10000,
	THRESH21 = 0xFF200000,
	THRESH26 = 0xF4000000,
	THRESH31 = 0x70000000;

std::string plaid::UTF8Encode(const String &str, char err)
{
	const Char *p = str.c_str();
	std::string res;
	Uint32 c, mask = ((sizeof(Char) >= 4) ? 0xFFFFFFFF : 0x0000FFFF);
	while (true)
	{
		c = (mask & Uint32(*p)); if (!c) break; ++p;
		if (c & THRESH07)
		{
			if (c & THRESH11)
			{
				if (c & THRESH16)
				{
					if (c & THRESH21)
					{
						if (c & THRESH26)
						{
							if (c & THRESH31)
							{
								//Overflow
								res.push_back(err);
								continue;
							}
							//31-bit; 6-byte
							res.push_back(F6 | ((c >> 30) & L1));
							//Extension 5
							res.push_back(F1 | ((c >> 24) & L6));
						}
						else
							//26-bit; 5-byte
							res.push_back(F5 | ((c >> 24) & L2));
						//Extension 4
						res.push_back(F1 | ((c >> 18) & L6));
					}
					else
						//21-bit; 4-byte
						res.push_back(F4 | ((c >> 18) & L3));
					//Extension 3
					res.push_back(F1 | ((c >> 12) & L6));
				}
				else
					//16-bit; 3-byte
					res.push_back(F3 | ((c >> 12) & L4));
				//Extension 2
				res.push_back(F1 | ((c >> 6) & L6));
			}
			else
				//11-bit; 2-byte
				res.push_back(F2 | ((c >> 6) & L5));
			//Extension 1
			res.push_back(F1 | (c & L6));
		}
		else
			//7-bit / 1-byte
			res.push_back(c);
	}

	return res;
}

String plaid::UTF8Decode(const std::string &str, Char err)
{
	const char *p = str.c_str();
	String res;
	Uint8 c; Uint32 oc, cont;
	while (true)
	{
		c = *p; if (!c) break; ++p;
		if (c & B7)
		{
			if (c & B6)
			{
				if (c & B5)
				{
					if (c & B4)
					{
						if (c & B3)
						{
							if (c & B2)
							{
								if (c & B1)
								{
									//Invalid...
									res.push_back(1);
									continue;
								}
								else
								{
									//6-byte; 31-bit
									oc = Uint32(c & L1) << 30;
									cont = 5;
								}
							}
							else
							{
								//5-byte; 26-bit
								oc = Uint32(c & L2) << 24;
								cont = 4;
							}
						}
						else
						{
							//4-byte; 21-bit
							oc = Uint32(c & L3) << 18;
							cont = 3;
						}
					}
					else
					{
						//3-byte; 16-bit
						oc = Uint32(c & L4) << 12;
						cont = 2;
					}
				}
				else
				{
					//2-byte; 11-bit
					oc = Uint32(c & L5) << 6;
					cont = 1;
				}
			}
			else
			{
				//Stray continuation; skip
				res.push_back(2);
				continue;
			}

			//Read continuation bytes
			while (cont)
			{
				c = *p;

				//Incomplete case
				if (!c) {res.push_back(3); break;}

				//Non-continuation character
				if ((c & F2) != F1) {res.push_back(4); continue;}

				//Add
				++p; --cont;
				oc += Uint32(c & L6) << (6 * cont);
			}
			res.push_back(oc);
		}
		else
		{
			//1-byte; 7-bit
			res.push_back(c);
		}
	}

	return res;
}

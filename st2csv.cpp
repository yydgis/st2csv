// test_rtcm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <map>
#include <string>

#define MAXFIELD 100

static int parse_fields(char* const buffer, char** val)
{
    char* p, * q;
    int n = 0;

    /* parse fields */
    for (p = buffer; *p && n < MAXFIELD; p = q + 1) {
        if (p == NULL) break;
        if ((q = strchr(p, ';')) || (q = strchr(p, '\n')) || (q = strchr(p, '\r'))) {
            val[n++] = p; *q = '\0';
        }
        else break;
    }
    return n;
}

static void set_output_file_name(const char* fname, const char* key, char* outfname)
{
    char filename[255] = { 0 }, outfilename[255] = { 0 };
    strcpy(filename, fname);
    char* temp = strrchr(filename, '.');
    if (temp) temp[0] = '\0';
    sprintf(outfname, "%s-%s", filename, key);
}


static FILE* set_output_file(const char* fname, const char* key)
{
    char filename[255] = { 0 };
    set_output_file_name(fname, key, filename);
    return fopen(filename, "w");
}

struct mount_t
{
    std::string name;
    double lat;
    double lon;
    int freq;
    std::string sys;
    std::string rcv;
    std::string country;
};

static void read_st(const char* fname, std::map<std::string, mount_t>& mMount)
{
    FILE* fLOG = fopen(fname, "rt");

    if (fLOG ==NULL) return;


    char buffer[255] = { 0 };
    char* val[MAXFIELD];

    int line_index = 0;

    while (fLOG && !feof(fLOG))
    {
        fgets(buffer, sizeof(buffer), fLOG);
        if (!(buffer[0] == 'S' && buffer[1] == 'T' && buffer[2] == 'R')) continue;
        int num = parse_fields(buffer, val);
        if (num < 14) continue;

        std::string name = std::string(val[1]);
        int freq = atoi(val[5]);
        std::string sys = std::string(val[6]);
        std::string country = std::string(val[8]);
        double lat = atof(val[9]);
        double lon = atof(val[10]);
        std::string rcv = std::string(val[13]);

        mount_t mount;
        mount.freq = freq;
        mount.country = country;
        mount.sys = sys;
        mount.rcv = rcv;
        mount.lat = lat;
        mount.lon = lon;
        mount.name = name;

        mMount[name] = mount;

    }
    if (fLOG) fclose(fLOG);
    return;
}

int main(int argc, const char* argv[])
{
    std::map<std::string, mount_t> mMount;
    if (argc > 1)
        read_st(argv[1], mMount);

    if (mMount.size()>0)
    {
        FILE* fOUT1 = set_output_file(argv[1], "-st.csv");
        if (fOUT1) fprintf(fOUT1, "name,lat,lon,country,constellation,receiver type\n");
        for (std::map<std::string, mount_t>::iterator pMount = mMount.begin(); pMount != mMount.end(); ++pMount)
        {
            if (fOUT1) fprintf(fOUT1, "%20s,%8.4f,%8.4f,%s,%s,%s\n", pMount->first.c_str(), pMount->second.lat, pMount->second.lon, pMount->second.country.c_str(), pMount->second.sys.c_str(), pMount->second.rcv.c_str());
        }
        if (fOUT1) fclose(fOUT1);
    }
    return 0;
}


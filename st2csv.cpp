// test_rtcm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <map>
#include <string>

#define MAXFIELD 100

static int parse_fields(char* const buffer, char** val, char key)
{
    char* p, * q;
    int n = 0;

    /* parse fields */
    for (p = buffer; *p && n < MAXFIELD; p = q + 1) {
        if (p == NULL) break;
        if ((q = strchr(p, key)) || (q = strchr(p, '\n')) || (q = strchr(p, '\r'))) {
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
        int num = parse_fields(buffer, val, ';');
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

int read_csv(const char* fname, std::map<std::string, mount_t>& mMount)
{
    FILE* fLOG = fopen(fname, "rt");

    if (fLOG == NULL) return 0;


    char buffer[255] = { 0 };
    char* val[MAXFIELD];

    int line_index = 0;

    while (fLOG && !feof(fLOG) && fgets(buffer, sizeof(buffer), fLOG))
    {
        ++line_index;
        if (line_index < 2) continue;

        int num = parse_fields(buffer, val, ',');
        if (num < 3) continue;

        std::string name = std::string(val[0]);
        double lat = atof(val[1]);
        double lon = atof(val[2]);

        mount_t mount;
        mount.lat = lat;
        mount.lon = lon;
        mount.name = name;

        mMount[name] = mount;

    }
    if (fLOG) fclose(fLOG);
    return (int)mMount.size();
}

static double lat2local(double lat, double* lat2north)
{
    double f_WGS84 = (1.0 / 298.257223563);
    double e2WGS84 = (2.0 * f_WGS84 - f_WGS84 * f_WGS84);
    double slat = sin(lat);
    double clat = cos(lat);
    double one_e2_slat2 = 1.0 - e2WGS84 * slat * slat;
    double Rn = 6378137.0 / sqrt(one_e2_slat2);
    double Rm = Rn * (1.0 - e2WGS84) / (one_e2_slat2);
    *lat2north = Rm;
    return Rn * clat;
}

int diff_csv(const char* fname1, const char* fname2)
{
    std::map<std::string, mount_t> mMount1;
    std::map<std::string, mount_t> mMount2;
    read_csv(fname1, mMount1);
    read_csv(fname2, mMount2);
    for (std::map<std::string, mount_t>::iterator pM1 = mMount1.begin(); pM1 != mMount1.end(); ++pM1)
    {
        double bestD = -1;
        double l2n = 0;
        double l2e = lat2local(pM1->second.lat * 3.1415926 / 180.0, &l2n);
        std::string bestM;
        for (std::map<std::string, mount_t>::iterator pM2 = mMount2.begin(); pM2 != mMount2.end(); ++pM2)
        {
            double dB = pM1->second.lat - pM2->second.lat;
            double dL = pM1->second.lon - pM2->second.lon;
            double dN = dB * 3.1415926 / 180.0 * l2n;
            double dE = dL * 3.1415926 / 180.0 * l2e;
            double currD = sqrt(dN * dN + dE * dE);
            if (bestD < 0 || currD < bestD)
            {
                bestD = currD;
                bestM = pM2->first;
            }
        }
        printf("%s,%7.2f,%7.2f,%14.4f,%s\n", pM1->first.c_str(), pM1->second.lat, pM1->second.lon, bestD, bestM.c_str());
    }
    return 0;
}

int main(int argc, const char* argv[])
{
    if (argc > 2)
    {
        diff_csv(argv[1], argv[2]);
    }
    else  if (argc > 1)
    {
        std::map<std::string, mount_t> mMount;
        read_st(argv[1], mMount);

        if (mMount.size() > 0)
        {
            FILE* fOUT1 = set_output_file(argv[1], "-st.csv");
            if (fOUT1) fprintf(fOUT1, "name,lat,lon,country,constellation,receiver type\n");
            for (std::map<std::string, mount_t>::iterator pMount = mMount.begin(); pMount != mMount.end(); ++pMount)
            {
                if (fOUT1) fprintf(fOUT1, "%20s,%8.4f,%8.4f,%s,%s,%s\n", pMount->first.c_str(), pMount->second.lat, pMount->second.lon, pMount->second.country.c_str(), pMount->second.sys.c_str(), pMount->second.rcv.c_str());
            }
            if (fOUT1) fclose(fOUT1);
        }
    }
    return 0;
}


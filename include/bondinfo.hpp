/**
 * bondinfo.hpp
 * Return the bond information we need in this project
 * Quanzhi Bi
 */

#ifndef BONDINFO_HPP
#define BONDINFO_HPP

#include <boost/date_time/gregorian/gregorian.hpp>
#include <map>
#include <string>
#include <vector>
#include <cmath>

#include "products.hpp"

class BondInfo {
   public:

    static std::vector<std::string> cusips;
    static std::map<std::string, boost::gregorian::date*> date_map;
    static std::map<std::string, Bond*> bond_map;

    
    // method to convert CUSIP from string to the coupon rate
    // data is from https://www.treasurydirect.gov/instit/instit.htm
    static double CUSIPToCoupon(string cusip) {
        if (cusip == "91282CAX9")  // 2Y
            return 0.00125;
        else if (cusip == "91282CBA80")  // 3Y
            return 0.00125;
        else if (cusip == "91282CAZ4")  // 5Y
            return 0.00375;
        else if (cusip == "91282CAY7")  // 7Y
            return 0.00625;
        else if (cusip == "91282CAV3")  // 10Y
            return 0.00875;
        else if (cusip == "912810ST6")  // 20Y
            return 0.01375;
        else if (cusip == "912810SS8")  // 30Y
            return 0.01625;
        else {  // wrong CUSIP
            std::cout << "BondInfo::CUSIPToCoupon: wrong CUSIP" << std::endl;
            return 0.0;
        }
    }

    // method to convert CUSIP from string to boost::gregorian::date
    static boost::gregorian::date* CUSIPToDate(string cusip) {
        // if (cusip == "91282CAX9")  // 2Y
        //     return new boost::gregorian::date(2022, Nov, 30);
        // else if (cusip == "91282CBA80")  // 3Y
        //     return new boost::gregorian::date(2023, Dec, 15);
        // else if (cusip == "91282CAZ4")  // 5Y
        //     return new boost::gregorian::date(2025, Nov, 30);
        // else if (cusip == "91282CAY7")  // 7Y
        //     return new boost::gregorian::date(2027, Nov, 30);
        // else if (cusip == "91282CAV3")  // 10Y
        //     return new boost::gregorian::date(2030, Nov, 15);
        // else if (cusip == "912810ST6")  // 20Y
        //     return new boost::gregorian::date(2040, Nov, 15);
        // else if (cusip == "912810SS8")  // 30Y
        //     return new boost::gregorian::date(2050, Nov, 15);
        // else {  // wrong CUSIP
        //     std::cout << "BondInfo::CUSIPToDate: wrong CUSIP" << std::endl;
        //     return nullptr;
        // }
        return date_map.find(cusip)->second;
    }

    // convert the price data from double to fractional notation
    static std::string FormatPrice(double price) {
        // I-xyz
        // where I is the integer part
        // xy from 0 to 31
        // z from 0 to 7
        // price = I + xy / 32 + z / 256
        int I = int(std::floor(price));
        int xy = int(std::floor(32*(price-I)));
        int z = int(std::floor(256*(price-I-xy/32.0)));
        std::string xy_str = std::to_string(xy);
        if (xy_str.size()==1) xy_str = "0" + xy_str;
        else if (xy_str.size() == 0) xy_str = "00";
        std::string ret = std::to_string(I)+"-"+xy_str+std::to_string(z);
        return ret;
    }

    // convert the price data from fractional notation to double
    static double CalculatePrice(std::string s) {
        int size = s.size();

        // a price string may have size = 6 or 7
        // add each digit times the decimal value of that position
        double price = (double)(s[size - 1] - '0') / 256.0;
        price += (double)(s[size - 2] - '0') / 32.0;
        price += 10.0 * (double)(s[size - 3] - '0') / 32.0;
        price += (double)(s[size - 5] - '0');
        price += 10.0 * double(s[size - 6] - '0');

        // deal with first digit.
        if (size == 7)
            price += 100.0;

        return price;
    }

    // return the CUSIPS
    static std::vector<std::string> GetCUSIP() {
        return {"91282CAX9", "91282CBA80",
                "91282CAZ4", "91282CAY7",
                "91282CAV3", "912810ST6", 
                "912810SS8"};
    }

    // return a bond product object via CUSIP
    static Bond* GetBond(std::string cusip) {
        // double coupon = CUSIPToCoupon(cusip);
        // auto maturityPtr = CUSIPToDate(cusip);
        // return new Bond(cusip, CUSIP, "T", coupon, *maturityPtr);
        return bond_map.find(cusip)->second;
    }

    // return the PV01 of the bond
    // We need yield curve to calculate the PV01
    // since we don't have it, we use T/100 instead
    static double GetPV01(std::string cusip) {
        
        if (cusip == "91282CAX9")  // 2Y
            return 0.02;
        else if (cusip == "91282CBA80")  // 3Y
            return 0.03;
        else if (cusip == "91282CAZ4")  // 5Y
            return 0.05;
        else if (cusip == "91282CAY7")  // 7Y
            return 0.07;
        else if (cusip == "91282CAV3")  // 10Y
            return 0.10;
        else if (cusip == "912810ST6")  // 20Y
            return 0.20;
        else if (cusip == "912810SS8")  // 30Y
            return 0.30;
        else {  // wrong CUSIP
            std::cout << "BondInfo::GetPV01: wrong CUSIP" << std::endl;
            return 0;
        }
    }

    // initialize the class
    static void init() {
        // first iniatilize the cusips
        cusips = {"91282CAX9", "91282CBA80",
                "91282CAZ4", "91282CAY7",
                "91282CAV3", "912810ST6", 
                "912810SS8"};
        // then initialize the date_map
        date_map.insert(make_pair("91282CAX9",  new boost::gregorian::date(2022, Nov, 30)));
        date_map.insert(make_pair("91282CBA80", new boost::gregorian::date(2023, Dec, 15)));
        date_map.insert(make_pair("91282CAZ4",  new boost::gregorian::date(2025, Nov, 30)));
        date_map.insert(make_pair("91282CAY7",  new boost::gregorian::date(2027, Nov, 30)));
        date_map.insert(make_pair("91282CAV3",  new boost::gregorian::date(2030, Nov, 15)));
        date_map.insert(make_pair("912810ST6",  new boost::gregorian::date(2040, Nov, 15)));
        date_map.insert(make_pair("912810SS8",  new boost::gregorian::date(2050, Nov, 15)));
        // them initialize the bond_map
        
        for (auto cusip: cusips) {
            double coupon = CUSIPToCoupon(cusip);
            auto maturityPtr = CUSIPToDate(cusip);
            auto bond = new Bond(cusip, CUSIP, "T", coupon, *maturityPtr);
            bond_map.insert(make_pair(cusip, bond));
        }
    }

    // delete the raw pointers
    static void clean() {
        for (auto p: date_map) {
            delete p.second;
        }

        for (auto p: bond_map) {
            delete p.second;
        }
    }
};

#endif
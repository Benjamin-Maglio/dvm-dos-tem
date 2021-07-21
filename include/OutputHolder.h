
#ifndef OUTPUTHOLDER_H
#define OUTPUTHOLDER_H

#include "temconst.h"
#include "layerconst.h"

class OutputHolder{
public:

//  OutputHolder();
//  ~OutputHolder();

  void clear();

  int months_held;
  int years_held;


  std::vector<int> ysd_for_output;

  std::vector<double> ald_for_output;
  std::vector<double> burnthick_for_output;
  std::vector<double> qdrain_for_output;
  std::vector<double> qinfil_for_output;
  std::vector<double> qrunoff_for_output;
  std::vector<double> snowthick_for_output;
  std::vector<double> swe_for_output;
  std::vector<double> watertab_for_output;


  //Variables by layer
  std::vector<double> rh_tot_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> rh_for_output;

  std::vector<std::array<double, MAX_SOI_LAY>> tlayer_for_output;
  std::vector<std::array<double, MAX_SOI_LAY>> vwclayer_for_output;

  //Variables by PFT
  std::vector<double> eet_tot_for_output;
  std::vector<std::array<double, NUM_PFT>> eet_for_output;
//  std::vector<std::array<double, NUM_PFT>> eet_test_vector;

  std::vector<double> pet_tot_for_output;
  std::vector<std::array<double, NUM_PFT>> pet_for_output;

  std::vector<double> trans_tot_for_output;
  std::vector<std::array<double, NUM_PFT>> trans_for_output;

  std::vector<std::array<double, NUM_PFT>> vegc_for_output;


  //Variables by PFT and Compartment both
  std::vector<double> gpp_tot_for_output;
  std::vector<std::array<double, NUM_PFT_PART>> gpp_part_for_output;
  std::vector<std::array<double, NUM_PFT>> gpp_pft_for_output;
//  std::vector<std::array<std::array<double, NUM_PFT_PART>, NUM_PFT>> gpp_for_output; 
  std::vector<std::array<std::array<double, NUM_PFT>, NUM_PFT_PART>> gpp_for_output; 

  std::vector<double> npp_tot_for_output;
  std::vector<std::array<double, NUM_PFT_PART>> npp_part_for_output;
  std::vector<std::array<double, NUM_PFT>> npp_pft_for_output;
  std::vector<std::array<std::array<double, NUM_PFT_PART>, NUM_PFT>> npp_for_output; 

};


#endif /* OUTPUTHOLDER_H */

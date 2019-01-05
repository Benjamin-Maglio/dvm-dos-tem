#include "../include/Richards.h"

#include "../include/TEMLogger.h"
extern src::severity_logger< severity_level > glg;

Richards::Richards() {
  //TSTEPMIN = 1.e-5;      //
  //TSTEPMAX = 0.2;
  //TSTEPORG = 0.1;
  //LIQTOLE = 0.05;  // tolearance is in fraction of 'maxliq' in a layer
  mindzlay = 0.005;
};

Richards::~Richards() {
};

void Richards::clearRichardsArrays(){
  for(int ii=0; ii<=MAX_SOI_LAY; ii++){

    //Incoming values
    Bsw[ii] = 0.0;
    ksat[ii] = 0.0;
    psisat[ii] = 0.0;

    //First round of calculated values
    k[ii] = 0.0;
    psi[ii] = 0.0;
    psiE[ii] = 0.0;
    theta[ii] = 0.0;
    thetasat[ii] = 0.0;
    z_h[ii] = 0.0;
    thetaE[ii] = 0.0;

    //Intermediate calculated values
    q_iminus1_n[ii] = 0.0;
    q_i_n[ii] = 0.0;
    eq7121[ii] = 0.0;
    eq7122[ii] = 0.0;
    eq7123[ii] = 0.0;
    eq7124[ii] = 0.0;
    eq7125[ii] = 0.0;

    eq7117[ii] = 0.0;
    eq7118[ii] = 0.0;
    eq7119[ii] = 0.0;
    eq7120[ii] = 0.0;

    coeffA[ii] = 0.0;
    coeffB[ii] = 0.0;
    coeffC[ii] = 0.0;
    coeffR[ii] = 0.0;

    //Solution
    deltathetaliq[ii] = 0.0;

    //unsorted TODO
    dzmm[ii] = 0.0;
    nodemm[ii] = 0.0;
    effliq[ii] = 0.0;
    effminliq[ii] = 0.0;
    effmaxliq[ii] = 0.0;
    qout[ii] = 0.0;
  }
};


//
void Richards::update(Layer *fstsoill, Layer* bdrainl,
                      const double & bdraindepth, const double & fbaseflow,
                      double trans[], const double & evap,
                      const double & infil, const double &ts) {
  //timestep = ts;
  drainl = bdrainl;

  //bdraindepth is provided by Soil_Env, which copies it from
  //ground->draindepth, which is set to the minimum of ald or watab
  //in Ground::setDrainL(). This may not be what we want. TODO
  z_watertab = bdraindepth * 1.e3;

  if (bdraindepth<=0.) {
    return; // the drainage occurs in the surface, no need to update the SM
  }

  //all fluxes already in mm/sec as input
  qinfil = infil;
  qevap  = evap;

//  for(int il=1; il<=MAX_SOI_LAY; il++) {
//    qtrans[il] = trans[il-1]; // trans[] starting from 0, while here all arrays starting from 1
//  }

  // initializing the arrays for use below
/*  for(int il=0; il<=MAX_SOI_LAY; il++) { // although starting 1, initialization from 0
 }*/

  qdrain = 0.;
  // loop for continuous unfrozen soil column section
  // in a soil profile, there may be a few or none
  Layer* currl=fstsoill;

  // excluding moss layer(s) for hydrological process due to
  // hydraulic parameters not validated, which causes oscillation
  // if no exclusion of moss layer, comment out this 'while' loop
  while (currl != NULL && currl->isMoss) {
    currl = currl->nextl;
  }

  //These are correct because the arrays are forced to 1-based indexing
  int topind = currl->solind;
  int drainind = drainl->solind;

  Layer* topsoill = currl;

  //Start of conversion to CLM 4.5

  //For now we're using a full day as a timestep, but introducing
  // a local variable in case that changes.
  double delta_t = SEC_IN_DAY;

  //Clear arrays before use
  clearRichardsArrays();

  //Prepare soil column - collect already known values into
  // arrays for ease of use, calculate the basic values needed
  // in the following (more complex) calculations.
  prepareSoilColumn(topsoill, bdraindepth);


  //For each thawed and partially thawed layer, run first round
  // of calculations. The results of these are needed by the
  // equations in the second round.
  while( (currl != NULL) && (currl->solind <= drainl->solind) ){

    int ind = currl->solind; //Correct because arrays are forced to 1-based

    //CLM 4.5 page 157
    //Theta - volumetric soil water content mm^3 water/ mm^3 soil
    //theta[ind] = effliq[ind] / DENLIQ / (currl->dz * 1.e3); //unitless
    theta[ind] = effliq[ind] / DENLIQ / (dzmm[ind]/1.e3); //unitless

    //Equation 7.94
    //psi_i - soil matric potential (mm)
    psi[ind] =  psisat[ind] * pow( (theta[ind] / thetasat[ind]),
                                   -Bsw[ind]);
    //Logging violations of the limits
    if(theta[ind]/thetasat[ind] < 0.01 ||
       theta[ind]/thetasat[ind] > 1.){
      BOOST_LOG_SEV(glg, err)<<"theta_i/thetasat_i out of range";
    }
    if(psi[ind] < -1e8){
      BOOST_LOG_SEV(glg, err)<<"psi["<<ind<<"] out of range: "<<psi[ind];
    }

    //Equation 7.129
    //thetaE_i - layer-average equilibrium volumetric water content 
    //This should only be used when the current layer and previous
    // layer are both above the water table.
    thetaE[ind] = thetasat[ind] * psisat[ind]
                / ( (z_h[ind] - z_h[ind-1]) * (1 - 1 / Bsw[ind]) )
                * (
                   pow( (psisat[ind] - z_watertab + z_h[ind])/psisat[ind],
                        (1-1/Bsw[ind]) )
                   - pow( (psisat[ind] - z_watertab + z_h[ind-1])/psisat[ind],
                        (1-1/Bsw[ind]) )
                  );
    //Separate equation for when that is not true.
    //TODO write separate equation

    //Equation 7.134
    //psiE_i - equilibrium soil matric potential
    psiE[ind] = psisat[ind] * pow( thetaE[ind]/thetasat[ind], -Bsw[ind] );
    //Logging violations of the limits
    if(thetaE[ind]/thetasat[ind] < 0.01){
      BOOST_LOG_SEV(glg, err)<<"thetaE_i/thetasat_i out of range";
    }
    if(psiE[ind] < -1e8){
      BOOST_LOG_SEV(glg, err)<<"psiE["<<ind<<"] out of range: "<<psiE[ind];
    }

    currl = currl->nextl; //Move down the soil column
  }

  //reset currl for the next round of calculations
  currl = topsoill;

  //For each thawed and partially thawed layer, run second round
  // of calculations.
  while( (currl != NULL) && (currl->solind <= drainl->solind) ){

    int ind = currl->solind; //Correct because arrays are forced to 1-based

    //Equation 7.89 (ignoring ice)
    //k: hydraulic conductivity
    //The 0.5 values should cancel - leaving them in for easy comparison
    //  with equations.
    //Given the large range of ksat between horizon types, we might want
    // to average the values instead of taking the value at the bottom
    // of the upper layer.
    //For all but drain layer
    if(ind<drainind){
      k[ind] = ksat[ind]
             * pow( (0.5*(theta[ind] + theta[ind+1]))
                  / (0.5*(thetasat[ind] + thetasat[ind+1])),
                   (2 * Bsw[ind] + 3) );
    }
    //Drain layer
    else{
      k[ind] = ksat[ind] * pow( theta[ind]/thetasat[ind],
                                (2 * Bsw[ind] + 3) );
    }

    //Equation 7.115
    //q_iminus1^n
    q_iminus1_n[ind] = -k[ind-1]
                     * ( (psi[ind-1] - psi[ind] + psiE[ind] - psiE[ind-1])
                         / (nodemm[ind] - nodemm[ind-1]) );

    //Equation 7.116
    //q_i^n
    q_i_n[ind] = -k[ind]
               * ( (psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind])
                   / (nodemm[ind+1] - nodemm[ind]) );

    //Equation 7.121
    //deltapsi_iminus1 / deltatheta_liq_iminus1
    eq7121[ind] = -Bsw[ind-1] * ( psi[ind-1] / theta[ind-1] );

    //Equation 7.122
    //deltapsi_i / deltatheta_liq_i
    eq7122[ind] = -Bsw[ind] * psi[ind] / theta[ind];

    //Equation 7.123
    //deltapsi_iplus1 / deltatheta_liq_iplus1
    eq7123[ind] = -Bsw[ind+1] * psi[ind+1] / theta[ind+1];

    //The first element of the following two equations is actually:
    //  (1 - (f_frz[ind-1] + f_frz[ind])/2)
    //However, because we restrict the Richards calculations to unfrozen
    // soil, this reduces to 1.

    //Equation 7.124
    //deltak[z_h_iminus1] / deltatheta_liq_iminus1
    //deltak[z_h_iminus1] / deltatheta_liq_i
    eq7124[ind] = 1
                * (2 * Bsw[ind-1] + 3) * ksat[ind-1]
                * pow( (0.5*(theta[ind-1] + theta[ind]))
                       / (0.5*(thetasat[ind-1] + thetasat[ind])),
                      (2 * Bsw[ind-1] + 2) )
                * ( 0.5 / (0.5*(thetasat[ind-1] + thetasat[ind])) );

    //Equation 7.125
    //deltak[z_h_i] / deltatheta_liq_i
    //deltak[z_h_i] / deltatheta_liq_iplus1
    eq7125[ind] = 1
                * (2 * Bsw[ind] + 3) * ksat[ind]
                * pow( (0.5*(theta[ind] + theta[ind+1]))
                       / (0.5*(thetasat[ind] + thetasat[ind+1])),
                      (2 * Bsw[ind] + 2) )
                * ( 0.5 / (0.5*(thetasat[ind] + thetasat[ind+1])) );


    //Equations 7.117-7.120 are not in numerical order because they
    // require some of the higher-numbered equations.
    //They also have sections replaced by the equations above, and
    // so do not precisely match the text
    // (i.e. eq7121[] instead of deltapsi[]/deltatheta_liq[])
    //Equation 7.117
    //deltaq_iminus1 / deltatheta_liq_iminus1
//    eq7117[ind] = - ( (k[z_h[ind-1]]/(z[ind]-z[ind-1]))
//                      * (deltapsi[ind-1]/deltatheta_liq[ind-1]) )
//                  - (deltak[z_h[ind-1]]/deltatheta_liq[ind-1])
//                  * ( psi[ind-1] - psi[ind] + psiE[ind] - psiE[ind-1]
//                      /(z[ind] - z[ind-1]) );

    //TODO explain why ind is not ind-1 (equation replacement)
    eq7117[ind] = - ( (k[ind-1]/(nodemm[ind]-nodemm[ind-1])) * eq7121[ind] )
                  - eq7124[ind] 
                  * ( psi[ind-1] - psi[ind] + psiE[ind] - psiE[ind-1]
                      /(nodemm[ind] - nodemm[ind-1]) );

    //Equation 7.118
    //deltaq_iminus1 / deltatheta_liq_i
//    eq7118[ind] = ( (k[z_h[ind-1]]/(z[ind]-z[ind-1]))
//                    * (deltapsi[ind]/deltatheta_liq[ind]) )
//                - (deltak[z_h[ind-1]]/deltatheta_liq[ind])
//                * ( psi[ind-1] - psi[ind] + psiE[ind] - psiE[ind-1]
//                    /(z[ind] - z[ind-1]) );

    eq7118[ind] = ( (k[ind-1]/(nodemm[ind]-nodemm[ind-1])) * eq7122[ind] )
                - eq7124[ind] 
                * ( psi[ind-1] - psi[ind] + psiE[ind] - psiE[ind-1]
                    /(nodemm[ind] - nodemm[ind-1]) );

    //Equation 7.119
    //deltaq_i / deltatheta_liq_i
//    eq7119[ind] = - ( (k[z_h[ind]]/(z[ind+1] - z[ind]))
//                      * (deltapsi[ind] / deltatheta_liq[ind]) )
//                  - (deltak[z_h[ind]]/deltatheta_liq[ind])
//                  * ( psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind]
//                      /(z[ind+1] - z[ind]) );

    eq7119[ind] = - ( (k[ind]/(nodemm[ind+1] - nodemm[ind])) * eq7122[ind] )
                  - eq7125[ind] 
                  * ( psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind]
                      /(nodemm[ind+1] - nodemm[ind]) );

    //Equation 7.120
    //deltaq_i / deltatheta_liq_iplus1
//    eq7120[ind] = ( (k[z_h[ind]]/(z[ind+1] - z[ind]))
//                    * (deltapsi[ind+1] / deltatheta_liq[ind+1]) )
//                - (deltak[z_h[ind]]/deltatheta_liq[ind+1])
//                * ( psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind]
//                    /(z[ind+1] - z[ind]) );
   //comment w/ explanation for equation replacement
   eq7120[ind] = (k[ind]/(nodemm[ind+1] - nodemm[ind])) * eq7123[ind]
                - eq7125[ind]  
                 * ( psi[ind] - psi[ind+1] + psiE[ind+1] - psiE[ind]
                     /(nodemm[ind+1] - nodemm[ind]) );

    //This is both the top active layer and the drain layer
    //The equations are a mix of the equations for top and drain layers,
    if(ind==topind && ind==drainind){

      //Equation 7.136
      coeffA[ind] = 0.0;

      //Equation 7.137. Uses 7.119
      coeffB[ind] = eq7119[ind] - dzmm[ind] / delta_t;

      //TODO clean up commenting
      //c_i = eq 7.138 or 0 depending on drainage category. Mult by drainage
      //Modified equation 7.138. Uses 7.120
      //This assumes a binary drainage class - 0 or 1. If a more flexible
      // approach is introduced, this will need modification.
      coeffC[ind] = eq7120[ind] * fbaseflow;
      
      coeffR[ind] = infil;
      //r_i = infl (should be zero if not the top soil layer). based on 7.139 and 7.147.   

    }
    //This is the top active layer
    else if(ind==topind){

      //Equation 7.136
      coeffA[ind] = 0.0; 

      //deltaz_i / deltat = layer thickness / 86,400 (because all data is in mm/s)
      //Equation 7.137. Uses 7.119
      coeffB[ind] = eq7119[ind] - dzmm[ind] / delta_t;

      //Equation 7.138. Uses 7.120
      coeffC[ind] = eq7120[ind];

      //Equation 7.139. Uses 7.116
      //Need to subtract evap here because we allow for it differently
      // than CLM. See section 7.3.3 for CLM approach. 
      // TODO verify the index modification - 1 or 2?
      //The sign convention between CLM 4.5 and ddt are different
      coeffR[ind] = infil - q_i_n[ind] + (evap + trans[ind+1]); 
    }
    //This is for the middle layers - neither top nor drain
    else if(ind>topind && ind<drainind){

      //Equation 7.140. Uses 7.117
      coeffA[ind] = - eq7117[ind];

      //Equation 7.141. Uses 7.119 and 7.118
      coeffB[ind] = eq7119[ind] - eq7118[ind] - dzmm[ind] / delta_t;

      //Equation 7.142. Uses 7.120
      coeffC[ind] = eq7120[ind];

      //Equation 7.143. Uses 7.115 and 7.116
      coeffR[ind] = q_iminus1_n[ind] - q_i_n[ind] + trans[ind];
    }
    //This is the drain layer
    else if(ind==drainind){

      //Equation 7.144. Uses 7.117
      coeffA[ind] = -eq7117[ind];

      //Equation 7.145. Uses 7.118
      coeffB[ind] = -eq7118[ind] - dzmm[ind] / delta_t;

      //Equation 7.146
      coeffC[ind] = 0.0; 

      //Equation 7.147. Uses 7.115
      coeffR[ind] = q_iminus1_n[ind] + trans[ind];
    }

    currl = currl->nextl; //Move down the soil column
  }

  //index of first active layer, number of active layers
  //four coefficients, then output array
  //in case of freezing front, first active layer is NOT indx0sl
  cn.tridiagonal(indx0al, numal, coeffA, coeffB, coeffC, coeffR, deltathetaliq);//water solver
  

  //do the next section for only active layers TODO
  currl = topsoill;
  while(currl->solind<indx0al){
    currl = currl->nextl;
  } 

  //Modify layer liquid by calculated change in liquid.
  for(int il=indx0al; il<indx0al+numal; il++){
    double minliq = effminliq[il];
    double maxliq = effmaxliq[il];

    currl->liq = deltathetaliq[il] + minliq;

    //Restricting layer liquid to range defined by min and max
    if(currl->liq<minliq){
      currl->liq = minliq;
    }

    if(currl->liq>maxliq){
      currl->liq = maxliq;
    }

    //Update hydraulic conductivity?
    //TODO

    currl = currl->nextl;
  }

////Old code:

/*
  while ( (currl != NULL) && (currl->solind <= drainl->solind) ) {
    // prepare arrays for calling Richards Equation's solver
    // for one continuous section of unfrozen soil column:
    // 'soilind' from indx0al:indx0al+numal
    prepareSoilNodes(currl, bdraindepth);

    //
    if (numal == 1) {// one layer: no need to iterate, tip-bucket approach enough
      int ind = indx0al;
      qin[ind] = 0.0;

      //if (indx0al == fstsoill->solind) 
      if (ind == indx0sl) {
        qin[ind] = infil - evap;
      }

      //because dzmm is in mm, no need to modify by density of water
      double s1 = effliq[ind]/dzmm[ind]/effporo[ind];
      double s2 = hksat[ind] * exp (-2*(zmm[ind]/1000.))
                  * pow((double)s1, (double)2*bsw[ind] +2);
      hk[ind] = s1*s2;

      if (ind == drainl->solind) {
        qout[ind] = hk[ind]*fbaseflow + trans[ind];
      } else {  //no drainage occurs if not in 'drainl'
        qout[ind] = trans[ind];
      }

      //effliq is in mm (kg/m^2)
      liqld[ind] = effliq[ind];
      //qin and qout are mm/sec, but the amount of liquid added to
      // liqld needs to be mm/day.
      liqld[ind] += (qin[ind]-qout[ind]) * SEC_IN_DAY;

      if (ind == drainl->solind) {
        // bottom drainage: mm/day->mm/sec
        qdrain += hk[ind] * fbaseflow * timestep / SEC_IN_DAY;
      }
    } else if(numal>1) {// iteration for unfrozen column with multiple layers
      iterate(trans, evap, infil, fbaseflow);
    } else { // if frozen (numal = 0), keep the current liq. water content
      liqld[currl->solind] = currl->liq;
    }

    //post-iteration
    // note: if not found a section of unfrozen soil column,
    // indx0al=-1 and numal=0,
    // then following 'while' and 'for' loops will not implement
    while (currl->solind<indx0al) {
      currl = currl->nextl;
    }

    for (int ind=indx0al; ind<indx0al+numal; ind++) {
      currl->liq = liqld[ind]+effminliq[ind];

      double minliq = effminliq[ind];

      if(currl->liq<minliq) {
        currl->liq=minliq;
      }

      double maxliq = effmaxliq[ind];

      if(currl->liq>maxliq) {
        currl->liq=maxliq;
      }

      // for output of hydraulic conductivity at each layer
      SoilLayer* sl=dynamic_cast<SoilLayer*>(currl);
      double ss = sl->getVolLiq()/(sl->poro-sl->getVolIce());
      double hcond = sl->hksat * pow((double)ss, (double)2*sl->bsw+2);
      currl->hcond = hcond*86400.; //unit: output in mm/day (hksat: mm/sec)
      // the following will move the 'currl' downward
      // ONLY for 'numal' when the 'for' loop goes
      currl=currl->nextl;
    }// end of post-iteration

    // the following will move the 'currl' downward one
    // layer for 'active layers' not find
    if (numal<1) {
      currl=currl->nextl;
    }
  } // end of whole soil layers loop for unfrozen column sections
*/

  // for layers above 'topsoill', e.g., 'moss',
  // if excluded from hydrological process
  currl = topsoill->prevl;

  while (currl!=NULL && currl->nextl!=NULL) {
    if (currl->indl<fstsoill->indl) {
      break;  // if no layer excluded, the 'while' loop will break here
    }

    double lwc = currl->nextl->getVolLiq();
    currl->liq = currl->dz*(1.0-currl->frozenfrac)*lwc*DENLIQ; //assuming same 'VWC' in the unfrozen portion as below
    currl=currl->prevl;
  }
};

// this will generate a continuous unfrozen column for soil water update
//This collects already-known values into arrays for ease of use, and
// calculates basic values needed in the more complex equations later
// in Richards.
void Richards::prepareSoilColumn(Layer* currsoill, const double & draindepth) {
  // it is assumed that all layers in Richards will be unfrozen,
  // i.e., from unfrozen 'topsoill' to ''drainl'
  Layer* currl = currsoill; // the first soil layer is 'topsoill'
  int ind = -1;
  indx0al = -1;
  numal = 0;

  while(currl!=NULL && currl->isSoil) {
    if(currl->solind>=indx0sl) {
      double dzunfrozen = currl->dz*(1.0-currl->frozenfrac);
      
      if (dzunfrozen>=mindzlay &&
          currl->solind <= drainl->solind) {// the last soil layer is 'drainl'
        if (indx0al<0) {
          indx0al=currl->solind;
        }

        ind = currl->solind;
        numal++;
        // if partially unfrozen layer: need to adjust 'dz',
        // 'z' for at top while not for bottom,
        // but not 'liq' (which always in unfrozen portion of layer)
        double frntdzadj = 1.;
        double frntzadj = 0.;
        // if drainage layer with watertable in it, 'dz' and
        // 'liq' need to be adjusted, but not 'z'
        double drdzadj = 1.;

        if (currl->frozen==0) {
          //Unfrozen fraction of the layer
          frntdzadj=1.0-currl->frozenfrac;

          if (currl->solind==indx0al || currl->solind==drainl->solind) {
            //Thickness of the frozen fraction of the layer
            frntzadj = fmax(0., currl->dz*currl->frozenfrac);
          }
        } else if (currl->solind == drainl->solind) {  //unfrozen layer but drainl, indicates a watertable in the layer
          //Fraction of the unsaturated portion. 
          drdzadj = (draindepth-currl->z)/currl->dz; //if 'draindepth' is inside of 'drainl'

        }

        double minvolliq = currl->minliq/DENLIQ/currl->dz;
        //effporo[ind] = fmax(0., currl->poro-minvolliq);
        thetasat[ind] = fmax(0., currl->poro-minvolliq);
        dzmm[ind] = currl->dz*1.e3*fmin(frntdzadj, drdzadj);

        //The soil node in a partially frozen layer will be calculated
        //differently if it's a thawing front vs a frozen front. If thawing,
        //the frozen slice of the layer will be a the bottom of the layer,
        //but if freezing, it will be at the top.
        bool thawing = false;

        //If there is no previous layer, determine status from next layer
        if(currl->prevl == NULL){
          //The layer below is frozen
          if(currl->nextl->frozen==1){ thawing = true; }
          //The layer below is thawed
          else{ thawing = false; }
        }
        //If there is a previous layer, determine status from it
        else{
          //The layer above is thawed/partially thawed
          if(currl->prevl->frozen <= 0){ thawing = true; }
          //The layer above is frozen
          else{ thawing = false; }
        }

        if(thawing){
          z_h[ind] = (currl->z + currl->dz - frntzadj)*1.e3;
          nodemm[ind] = (currl->z + currl->dz - frntzadj)*1.e3 - 0.5*dzmm[ind];
        }
        else{
          z_h[ind] = (currl->z + currl->dz)*1.e3;
          nodemm[ind]  = (currl->z+frntzadj)*1.e3 + 0.5 *dzmm[ind]; // the node depth (middle point of a layer)
        }
 
        //This is a weird formulation, but works out?
        effminliq[ind] = currl->minliq*fmin(frntdzadj, drdzadj);
        //effmaxliq[ind] = (effporo[ind]*dzmm[ind]);
        effmaxliq[ind] = thetasat[ind] * dzmm[ind];

        //This is also a weird formulation, but works out?
        effliq[ind] = fmax(0.0, currl->liq*drdzadj-effminliq[ind]);

        if (effliq[ind]<0. || effminliq[ind]<0. || effmaxliq[ind]<0.) {
          BOOST_LOG_SEV(glg, warn) << "Richards::prepareSoilColumn(..) "
                                   << "Effective liquid is less than zero!";
        }

        psisat[ind] = currl->psisat;
        ksat[ind] = currl->hksat;
        Bsw[ind]   = currl->bsw;
      } else {
        break;
      }
    } else {
      break;
    }

    currl= currl->nextl;
  }
};

/*
void Richards::iterate(const double trans[], const double & evap,
                       const double & infil, const double & fbaseflow) {
  //
  tschanged = true;
  itsum = 0;
  tleft = 1.;    // at beginning of update, tleft is one timestep

  if(infil>0.) {
    TSTEPORG =TSTEPMAX/20.;
  } else {
    TSTEPORG =TSTEPMAX;
  }

  tstep = TSTEPORG;

  for(int il=indx0al; il<indx0al+numal; il++) {
    liqid[il] = effliq[il]; // liq at the begin of one day
    liqld[il] = effliq[il]; // the last determined liq
  }

  qdrain = 0.;   // for accumulate bottom drainage (mm/day)

  while(tleft>0.0) {
    for(int il=indx0al; il<indx0al+numal; il++) {
      liqis[il] = liqld[il];
    }

    //find one solution for one fraction of timestep
    int st = updateOnethTimeStep(fbaseflow);

    if(st==0 || (st!=0 && tstep<=TSTEPMIN)) {  //advance to next timestep
      qdrain += qout[numal]*tstep*timestep; //unit: mm/s*secs
      tleft -= tstep;

      // find the proper timestep for rest period
      if(!tschanged) { // if timestep has not been changed during last time step
        if(tstep<TSTEPMAX) {
          tstep = TSTEPORG;
          tschanged = true;
        }
      } else {
        tschanged =false;
      }

      // make sure tleft is greater than zero
      tstep = fmin(tleft, tstep);

      if(tstep<=0) {  //starting the next iterative-interval
        qdrain = 0.;
      }
    } else {
      tstep = tstep/2.0;   // half the iterative-interval

      if(tstep < TSTEPMIN) {
        tstep = TSTEPMIN;
      }

      tschanged = true;
    }
  } // end of while
};
*/
/*
int Richards::updateOnethTimeStep(const double &fbaseflow) {
  int status =-1;

  for(int i=indx0al; i<indx0al+numal; i++) {
    liqii[i] = liqis[i];
  }

  status = updateOneIteration(fbaseflow);

  if(status==0 || tstep<=TSTEPMIN) { // success OR at the min. tstep allowed
    for(int i=indx0al; i<indx0al+numal; i++) {
      liqld[i] = liqit[i];
    }
  }

  return status;
};
*/
/*
int Richards::updateOneIteration(const double &fbaseflow) {
  double effporo0;
  double effporo2;
  double volliq  = 0.;
  double volliq2 = 0.;;
  double s1;
  double s2;
  double s_node;
  double wimp = 0.001; // mimumum pore for water to exchange between two layers
  double smpmin = -1.e8;
  double dt =tstep*timestep;
  itsum++;

  //Yuan: k-dk/dw-h relationships for all soil layers
  for (int indx=indx0al; indx<indx0al+numal; indx++) {
    effporo0 = effporo[indx];
    volliq = fmax(0., liqii[indx]/dzmm[indx]);
    volliq = fmin(volliq, effporo0);

    if(indx==indx0al+numal-1) {
      s1 = volliq/fmax(wimp, effporo0);
      s2 = hksat[indx] * exp (-2.0*(zmm[indx]/1000.0))
           * pow(s1, 2.0*bsw[indx]+2.0);
      hk[indx] = s1*s2;
      dhkdw[indx] = (2.0*bsw[indx]+3.0)*s2*0.5/fmax(wimp, effporo0);
    } else {
      effporo2 = effporo[indx+1];
      volliq2 = fmax(0., liqii[indx+1]/dzmm[indx+1]);
      volliq2 = fmin(volliq2, effporo2);

      if(effporo0<wimp || effporo2<wimp) {
        hk[indx] = 0.;
        dhkdw[indx] = 0.;
      } else {
        s1 =(volliq2+volliq)/(effporo2+effporo0);
        s2 = hksat[indx+1] * exp (-2.0*(zmm[indx+1]/1000.0))
             * pow(s1, 2.0*bsw[indx+1]+2.0);
        hk[indx] = s1*s2;
        dhkdw[indx] = (2.*bsw[indx]+3.0)*s2*0.5/effporo2;
      }
    }

    if (hk[indx]>=numeric_limits<double>::infinity()
        || dhkdw[indx]>=numeric_limits<double>::infinity()) {
      BOOST_LOG_SEV(glg, warn) << "'hk' or 'dhkdw' is out of bounds!";
    }

    if (volliq>1.0 || volliq2>1.0) {
      BOOST_LOG_SEV(glg, warn) << "vwc is out of bounds! (volliq or volliq2 > 1.0)";
    }

    //
    s_node = volliq/fmax(wimp, effporo0);
    s_node = fmax(0.001, (double)s_node);
    s_node = fmin(1.0, (double)s_node);
    smp[indx] = psisat[indx]*pow(s_node, -bsw[indx]);
    smp[indx] = fmax(smpmin, smp[indx]);
    dsmpdw[indx]= -bsw[indx]*smp[indx]/(s_node*fmax(wimp,effporo0));

    //
    if (smp[indx]>=numeric_limits<double>::infinity()
        || dsmpdw[indx]>=numeric_limits<double>::infinity()) {
      BOOST_LOG_SEV(glg, warn) << "smp[<<"<<indx<<"] or dsmpdw["<<indx<<"] is infinity!";
    }
  }

  // preparing matrice for solution
  double den, num;
  double dqodw1, dqodw2, dqidw0, dqidw1;
  double sdamp =0.;
  int ind=indx0al;

  if(numal>=2) {
    // layer 1
    qin[ind] = 0.;

    if (ind == indx0sl) {//for first soil layer: infiltration/evaporation occurs
      qin[ind] = qinfil -qevap;
    }

    den = zmm[ind+1]-zmm[ind];
    num = smp[ind+1]-smp[ind]-den;
    qout[ind] = -hk[ind] * num/den;
    dqodw1 = -(-hk[ind]*dsmpdw[ind] + num*dhkdw[ind])/den;
    dqodw2 = -(hk[ind]*dsmpdw[ind+1] + num*dhkdw[ind])/den;
    rmx[ind] = qin[ind] - qout[ind] - qtrans[ind];
    amx[ind] = 0.;
    bmx[ind] = dzmm[ind] *(sdamp +1/dt) + dqodw1;
    cmx[ind] = dqodw2;

    if (numal>2) {
      for(ind=indx0al+1; ind<indx0al+numal-1; ind++) { // layer 2 ~ the second last bottom layer
        den = zmm[ind]-zmm[ind-1];
        num = smp[ind]-smp[ind-1] -den;
        qin[ind] = -hk[ind-1]*num/den;
        dqidw0 = -(-hk[ind-1]*dsmpdw[ind-1] + num* dhkdw[ind-1])/den;
        dqidw1 = -(hk[ind-1]*dsmpdw[ind] + num* dhkdw[ind-1])/den;
        den = zmm[ind+1]-zmm[ind];
        num = smp[ind+1]-smp[ind] -den;
        qout[ind] = -hk[ind] * num/den;
        dqodw1 = -(-hk[ind]*dsmpdw[ind] + num* dhkdw[ind])/den;
        dqodw2 = -(hk[ind]*dsmpdw[ind+1] + num* dhkdw[ind])/den;
        rmx[ind] = qin[ind] -qout[ind] -qtrans[ind];
        amx[ind] =-dqidw0;
        bmx[ind] = dzmm[ind] /dt - dqidw1 + dqodw1;
        cmx[ind] = dqodw2;

        if (amx[ind] != amx[ind] || bmx[ind] != bmx[ind] ||
            cmx[ind] != cmx[ind] || rmx[ind] != rmx[ind]) {
          BOOST_LOG_SEV(glg, warn) << "amx, cmx, bmx, or rmx at index "
                                   << ind << " is NaN!";
        }
      }
    }

    //bottom layer
    ind = indx0al+numal-1;
    den = zmm[ind]-zmm[ind-1];
    num = smp[ind]-smp[ind-1]-den;
    qin[ind] = -hk[ind-1]*num/den;
    dqidw0 = -(-hk[ind-1]*dsmpdw[ind-1] + num* dhkdw[ind-1])/den;
    dqidw1 = -(hk[ind-1]*dsmpdw[ind] + num* dhkdw[ind-1])/den;
    dqodw1 = dhkdw[ind];
    qout[ind] = 0.;   //no drainage occurs if not in 'drainl'

    if (ind==drainl->solind) {
      qout[ind] = hk[ind]*fbaseflow;    //free bottom drainage assumed
    }

    rmx[ind] = qin[ind] -qout[ind] -qtrans[ind];
    amx[ind] = -dqidw0;
    bmx[ind] = dzmm[ind]/dt - dqidw1 + dqodw1;
    cmx[ind] = 0.;
  }

  cn.tridiagonal(indx0al, numal, amx, bmx,cmx,rmx, dwat);  //solution

  // soil water for each layer after one iteration
  for(int il=indx0al; il<indx0al+numal; il++) {
    liqit[il] = liqii[il] + dzmm[il] * dwat[il];

    if(liqit[il]!=liqit[il]) {
      BOOST_LOG_SEV(glg, warn) << "Richards::updateOneIteration(..), water is NaN!";
    }

    if (liqit[il]>=numeric_limits<double>::infinity()) {
      BOOST_LOG_SEV(glg, warn) << "liqit["<<il<<"] is greater than infinity.";
    }
  }

  //check the iteration result to determine if need to continue
  for(int il=indx0al; il<indx0al+numal; il++) {
    /* // the '-1' and '-2' status appear causing yearly unstablitity - so removed
          if(liqit[il]<0.0){
            return -1;    // apparently slow down the iteration very much during drying
          }
          if(liqit[il]>effmaxliq[il]){
            return -2;    // apparently slow down the iteration very much during wetting
          }
    //
    if(fabs((liqit[il]-liqii[il])/effmaxliq[il])>LIQTOLE) {
      return -3;
    }
  }

  return 0;
};
*/


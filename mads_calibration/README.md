MADS-TEM parameter calibration 
===========================================

The autocalibration (AC) process is focused on matching average above- and below-ground carbon and nitrogen stocks and fluxes. We match mean annual observed values during the equilibrium run period to ensure that model represents the history of a given site. All parameters and observations are vectors, where an element of a vector represents a given plant functional type (PFT) within a given vegetation community type or a subsurface parameter. The calibration workflow consists of four steps. The first three steps calibrate above-ground carbon and nitrogen fluxes, and the fouth step is designed to calibrate below-ground stocks. To start the calibration process in MADS, we provide an initial set of parameter values called initial guesses (see yaml files). The initial guess usually comes from the previous set of values for a similar vegetation community type. MADS allows setting ranges for each element of the initial guess vector. At each step, we can run one or multiple (R) calibration runs to test for the overall method convergence. The letter (R) stands for random indicating that values in the initial guess vector are randomly perturbated. MADS allows a combination of multiple parameters and target values in one calibration, accounting for the combined effect of multiple correlated parameters on observations. Starting from step 2, we combine (C) multiple parameters and target values to study the combined effect of multiple correlated parameters on observations. We can also run both (C) and (R) at the same time. The calibration process is scalable and can be run in parallel on multiple processors. 


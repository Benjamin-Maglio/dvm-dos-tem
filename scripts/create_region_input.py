#!/usr/bin/env python


from subprocess import call

import argparse
import textwrap
import os

import netCDF4
import numpy as np

from osgeo import gdal

#some description of what region wanted
#for now, keep to a rectangular requirement?
#Maintain CF and COARDS standards

#Select y,x or lat,lon bounding box coordinates for use


#Data should be in a rectangular (grid) layout
#NetCDF
#Conforms to CF & COARDS standards
#Geospatial information must be with the file. Each file should have variables for Lat and Lon each defined in terms of the dimensions of (y,x) where (y,x) are the rectangular grid coordinates.


#(0,0) pixel is hardcoded to the exact values from Toolik for testing.


def make_fire_dataset(fname, sizey=10, sizex=10):
  '''Generate a file representing fire information'''

  print "Creating a fire classification file, %s by %s pixels." % (sizey, sizex)
  ncfile = netCDF4.Dataset(fname, mode='w', format='NETCDF4')

  Y = ncfile.createDimension('Y', sizey)
  X = ncfile.createDimension('X', sizex)
  fri = ncfile.createVariable('fri', np.int, ('Y','X',))
  fire_year_vector = ncfile.createVLType(np.int, 'fire_year_vector')
  fire_years = ncfile.createVariable('fire_years', fire_year_vector, ('Y','X'))
  fire_sizes = ncfile.createVariable('fire_sizes', fire_year_vector, ('Y','X'))

  print " --> NOTE: Filling FRI with random data!"
  fri[:] = np.random.uniform(low=1, high=7, size=(10, 10))

  print " --> NOTE: Setting FRI for pixel 0,0 to 1000!"
  fri[0,0] = 1000

  print " --> NOTE: Filling the fire_year and fire_sizes with random data!"
  yr_data = np.empty(sizey * sizex, object)
  sz_data = np.empty(sizey * sizex, object)
  for n in range(sizey * sizex):
    yr_data[n] = np.array(sorted(np.random.randint(1900, 2006, np.random.randint(0,10,1))), dtype=np.int)
    sz_data[n] = np.random.randint(0,100,len(yr_data[n]))
    #numpy.arange(random.randint(1,10),dtype='int32')+1
    
  yr_data = np.reshape(yr_data,(sizey,sizex))
  sz_data = np.reshape(sz_data,(sizey,sizex))

  print " --> NOTE: Check on a few pixels?"
  print "  (0,0)", yr_data[0,0], "-->", sz_data[0,0]
  print "  (0,1)", yr_data[0,1], "-->", sz_data[0,1]
  print "  (9,9)", yr_data[9,9], "-->", sz_data[9,9]
    
  fire_years[:] = yr_data
  fire_sizes[:] = sz_data

  ncfile.close()   


def make_veg_classification(fname, sizey=10, sizex=10):
  '''Generate a file representing veg classification.'''

  print "Creating a vegetation classification file, %s by %s pixels." % (sizey, sizex)
  ncfile = netCDF4.Dataset(fname, mode='w', format='NETCDF4')

  Y = ncfile.createDimension('Y', sizey)
  X = ncfile.createDimension('X', sizex)
  veg_class = ncfile.createVariable('veg_class', np.int, ('Y', 'X',))

  print " --> NOTE: Filling with random data!"
  veg_class[:] = np.random.uniform(low=1, high=7, size=(10,10))

  print " --> NOTE: Setting pixel 0,0 to 4"
  veg_class[0,0] = 4
    
  ncfile.close()


def make_drainage_classification(fname, sizey=10, sizex=10):
  '''Generate a file representing drainage classification.'''
  print "Creating a drainage classification file, %s by %s pixels." % (sizey, sizex)
  ncfile = netCDF4.Dataset(fname, mode='w', format='NETCDF4')

  Y = ncfile.createDimension('Y', sizey)
  X = ncfile.createDimension('X', sizex)
  drainage_class = ncfile.createVariable('drainage_class', np.int, ('Y', 'X',))

  print " --> NOTE: Filling with random data!"
  drainage_class[:] = np.random.uniform(low=1, high=7, size=(10,10))

  print " --> NOTE: Setting 0,0 pixel to zero!"
  drainage_class[0,0] = 0

  ncfile.close()


def make_run_mask(filename, sizey=10, sizex=10):
  '''Generate a file representing the run mask'''

  print "Creating a run_mask file, %s by %s pixels." % (sizey, sizex)
  ncfile = netCDF4.Dataset(filename, mode='w', format='NETCDF4')

  Y = ncfile.createDimension('Y', sizey)
  X = ncfile.createDimension('X', sizex)
  run = ncfile.createVariable('run', np.int, ('Y', 'X',))

  print " --> NOTE: Turning off all pixels except 0,0."
  run[:] = np.zeros((10,10))
  run[0,0] = 1
    
  ncfile.close()


def copy_co2_to_new_style(filename):
  '''Generates a co2 file for dvmdostem from the old sample data'''

  print "Creating a co2 file..."
  new_ncfile = netCDF4.Dataset(filename, mode='w', format='NETCDF4')

  # Dimensions
  yearD = new_ncfile.createDimension('year', None) # append along time axis
    
  # Coordinate Variable
  yearV = new_ncfile.createVariable('year', np.int, ('year',))
    
  # Data Variables
  co2 = new_ncfile.createVariable('co2', np.float32, ('year',))
    

  print " --> NOTE: Hard-coding the values that were just ncdumped from the old file..."
  co2[:] = [ 296.311, 296.661, 297.04, 297.441, 297.86, 298.29, 298.726, 299.163,
    299.595, 300.016, 300.421, 300.804, 301.162, 301.501, 301.829, 302.154, 
    302.48, 302.808, 303.142, 303.482, 303.833, 304.195, 304.573, 304.966, 
    305.378, 305.806, 306.247, 306.698, 307.154, 307.614, 308.074, 308.531, 
    308.979, 309.401, 309.781, 310.107, 310.369, 310.559, 310.667, 310.697, 
    310.664, 310.594, 310.51, 310.438, 310.401, 310.41, 310.475, 310.605, 
    310.807, 311.077, 311.41, 311.802, 312.245, 312.736, 313.27, 313.842, 
    314.448, 315.084, 315.665, 316.535, 317.195, 317.885, 318.495, 318.935, 
    319.58, 320.895, 321.56, 322.34, 323.7, 324.835, 325.555, 326.55, 
    328.455, 329.215, 330.165, 331.215, 332.79, 334.44, 335.78, 337.655, 
    338.925, 340.065, 341.79, 343.33, 344.67, 346.075, 347.845, 350.055, 
    351.52, 352.785, 354.21, 355.225, 356.055, 357.55, 359.62, 361.69, 
    363.76, 365.83, 367.9, 368, 370.1, 372.2, 373.6943, 375.3507, 377.0071, 
    378.6636, 380.5236, 382.3536, 384.1336 ]

  yearV[:] = [ 1901, 1902, 1903, 1904, 1905, 1906, 1907, 1908, 1909, 1910, 1911,
    1912, 1913, 1914, 1915, 1916, 1917, 1918, 1919, 1920, 1921, 1922, 1923, 
    1924, 1925, 1926, 1927, 1928, 1929, 1930, 1931, 1932, 1933, 1934, 1935, 
    1936, 1937, 1938, 1939, 1940, 1941, 1942, 1943, 1944, 1945, 1946, 1947, 
    1948, 1949, 1950, 1951, 1952, 1953, 1954, 1955, 1956, 1957, 1958, 1959, 
    1960, 1961, 1962, 1963, 1964, 1965, 1966, 1967, 1968, 1969, 1970, 1971, 
    1972, 1973, 1974, 1975, 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 
    1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 
    1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 
    2008, 2009 ]

  new_ncfile.close()


def create_empty_climate_nc_file(filename, sizey=10, sizex=10):
  '''Creates an empty climate file for dvmdostem; y,x grid, time unlimited.'''

  print "Creating an empty climate file..."
  ncfile = netCDF4.Dataset(filename, mode="w", format='NETCDF4')

  # Dimensions for the file.
  time_dim = ncfile.createDimension('time', None) # append along time axis
  Y = ncfile.createDimension('Y', sizey)
  X = ncfile.createDimension('X', sizex)

  # Coordinate Variables
  Y = ncfile.createVariable('Y', np.int, ('Y',))
  X = ncfile.createVariable('X', np.int, ('X',))
  Y[:] = np.arange(0, sizey)
  X[:] = np.arange(0, sizex)

  # 'Spatial Refefence' variables (?)
  lat = ncfile.createVariable('lat', np.float32, ('Y', 'X',))
  lon = ncfile.createVariable('lon', np.float32, ('Y', 'X',))

  # Create data variables
  #co2 = ncfile.createVariable('co2', np.float32, ('time')) # actually year
  temp_air = ncfile.createVariable('tair', np.float32, ('time', 'Y', 'X',))
  precip = ncfile.createVariable('precip', np.float32, ('time', 'Y', 'X',))
  nirr = ncfile.createVariable('nirr', np.float32, ('time', 'Y', 'X',))
  vapor_press = ncfile.createVariable('vapor_press', np.float32, ('time', 'Y', 'X',))

  ncfile.close()



if __name__ == '__main__':

  parser = argparse.ArgumentParser(
    formatter_class = argparse.RawDescriptionHelpFormatter,

      description=textwrap.dedent('''\
        stuff
        '''),

      epilog=textwrap.dedent('''\
        '''),
  )

  parser.add_argument('--dim', default=10, type=int,
                      help="Width and height of square selection")

  parser.add_argument('--tifs', default="../../snap-data",
                      help="Directory containing input TIF directories")

  parser.add_argument('--outdir', default=".",
                      help="Directory for netCDF output files")

  parser.add_argument('--loc', default="Toolik",
                      help="Location of data set (for dir naming)")

  print "Parsing command line arguments";
  args = parser.parse_args()
  print args


  x_dim = args.dim;
  y_dim = args.dim;

  tif_dir = args.tifs;

  # Like this: somedirectory/somelocation_NxM
  out_dir = os.path.join(args.outdir, "%s_%sx%s" % (args.loc, x_dim, y_dim))
  print "Will be (over)writing files to: ", out_dir


#Pick bounding box coordinates to use with gdal_translate for subsetting the AIEM domain data files from SNAP. Current files from SNAP are Alaska Albers, 1km pixel size

#Specifying the "creation option" means that special variables will be written to the new netcdf file mapping row/column coordinates to lat/lon

#The following two calls must still be done manually

  lonlat_settings = '\"WRITE_LONLAT=YES\"'
  print lonlat_settings
 
#  call(['gdal_translate', '-of', 'netCDF', '-co', lonlat_settings,
#        tif_dir + '/tas_mean_C_iem_cccma_cgcm3_1_sresa1b_2001_2100/tas_mean_C_iem_cccma_cgcm3_1_sresa1b_01_2001.tif',
#        'sc_temporary_with_lonlat.nc']);

#  call(['gdal_translate', '-of', 'netCDF', '-co', '\"WRITE_LONLAT=YES\"',
#        'tas_mean_C_iem_cccma_cgcm3_1_sresa1b_01_2001.tif',
#        'sc_temporary_with_lonlat.nc']);

#  gdal_translate -of netCDF -co "WRITE_LONLAT=YES" \
#    -co GDAL_NETCDF_BOTTOMUP=YES -srcwin 915 292 10 10 \
#    temporary_with_lonlat.nc temp_subset_with_lonlat.nc



  if not os.path.exists(out_dir):
    os.makedirs(out_dir)


  make_fire_dataset(os.path.join(out_dir, "script-new-fire-dataset.nc"), sizey=y_dim, sizex=x_dim)

  make_veg_classification(os.path.join(out_dir, "script-new-veg-dataset.nc"), sizey=y_dim, sizex=x_dim)

  make_drainage_classification(os.path.join(out_dir, "script-new-drainage-dataset.nc"), sizey=y_dim, sizex=x_dim)

  make_run_mask(os.path.join(out_dir, "script-run-mask.nc"), sizey=y_dim, sizex=x_dim)

  #Copy CO2 data to a new file that follows proper standards/formatting
  copy_co2_to_new_style(out_dir + "/script-new-co2-dataset.nc");

  #Create empty file to copy data into
  create_empty_climate_nc_file(out_dir + "/script-projected-climate-dataset.nc", sizey=y_dim, sizex=x_dim)

####
  #Open the 'temporary' dataset
  temp_subset_with_lonlat = netCDF4.Dataset("temp_subset_with_lonlat.nc", mode='r')

  #Open the new file for appending
  new_climatedataset = netCDF4.Dataset(out_dir + "/script-projected-climate-dataset.nc", mode='a')

  #Insert lat/lon from temp file into the new file
  lat = new_climatedataset.variables['lat']
  lon = new_climatedataset.variables['lon']
  lat[:] = temp_subset_with_lonlat.variables['lat'][:]
  lon[:] = temp_subset_with_lonlat.variables['lon'][:]

  new_climatedataset.close()
  temp_subset_with_lonlat.close()
####

####
  YEARS=99
  #Populate input file with data from TIFs
  with netCDF4.Dataset(out_dir + '/script-projected-climate-dataset.nc', mode='a') as new_climatedataset:

    for yridx, year in enumerate(range(2001, 2001+YEARS)):
      for midx, month in enumerate(range (1,13)): # Note 1 based month!
        print year, month
        # TRANSLATE TO NETCDF
        # The curly braces are needed to run the shell command from w/in
        # ipython and have the variable exansion with year and month
        # work out alright
        print "Converting tif --> netcdf..."
        call(['gdal_translate', '-of', 'netCDF',
              tif_dir + '/tas_mean_C_iem_cccma_cgcm3_1_sresa1b_2001_2100/tas_mean_C_iem_cccma_cgcm3_1_sresa1b_%02d_%04d.tif' % (month,year),
              'script-temporary_tair.nc'])


        call(['gdal_translate', '-of', 'netCDF',
              tif_dir + '/rsds_mean_MJ-m2-d1_iem_cccma_cgcm3_1_sresa1b_2001_2100/rsds_mean_MJ-m2-d1_iem_cccma_cgcm3_1_sresa1b_%02d_%04d.tif' % (month, year),
              'script-temporary_rsds.nc'])

        call(['gdal_translate', '-of', 'netCDF',
              tif_dir + '/pr_total_mm_iem_cccma_cgcm3_1_sresa1b_2001_2100/pr_total_mm_iem_cccma_cgcm3_1_sresa1b_%02d_%04d.tif' % (month, year),
              'script-temporary_pr.nc'])

        call(['gdal_translate', '-of', 'netCDF',
              tif_dir + '/vap_mean_hPa_iem_cccma_cgcm3_1_sresa1b_2001_2100/vap_mean_hPa_iem_cccma_cgcm3_1_sresa1b_%02d_%04d.tif' % (month, year),
              'script-temporary_vapo.nc'])


        print "Subsetting...."
        call(['gdal_translate', '-of', 'netCDF', '-srcwin',
              '915', '292', '10', '10',
              'script-temporary_tair.nc', 'script-temporary_tair2.nc'])

        call(['gdal_translate', '-of', 'netCDF', '-srcwin',
              '915', '292', '10', '10',
              'script-temporary_rsds.nc', 'script-temporary_rsds2.nc'])

        call(['gdal_translate', '-of', 'netCDF', '-srcwin',
              '915', '292', '10', '10',
              'script-temporary_pr.nc', 'script-temporary_pr2.nc'])

        call(['gdal_translate', '-of', 'netCDF', '-srcwin',
              '915', '292', '10', '10',
              'script-temporary_vapo.nc', 'script-temporary_vapo2.nc'])


        print "Writing subset's data to new files..."
        with netCDF4.Dataset('script-temporary_tair2.nc', mode='r') as t2:
          # Grab the lat and lon from the temporary file
          tair = new_climatedataset.variables['tair']
          tair[yridx*12+midx] = t2.variables['Band1'][:]

        with netCDF4.Dataset('script-temporary_rsds2.nc', mode='r') as t2:
          # Grab the lat and lon from the temporary file
          nirr = new_climatedataset.variables['nirr']
          nirr[yridx*12+midx] = t2.variables['Band1'][:]
                
        with netCDF4.Dataset('script-temporary_pr2.nc', mode='r') as t2:
          # Grab the lat and lon from the temporary file
          prec = new_climatedataset.variables['precip']
          prec[yridx*12+midx] = t2.variables['Band1'][:]

        with netCDF4.Dataset('script-temporary_vapo2.nc', mode='r') as t2:
          # Grab the lat and lon from the temporary file
          vapo = new_climatedataset.variables['vapor_press']
          vapo[yridx*12+midx] = t2.variables['Band1'][:]
                
  print "Done appending. Closing the new file"
####

#end Main






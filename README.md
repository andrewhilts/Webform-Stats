##Webform Stats

###Basic info

name            = "Webform Stats"  
description     = "Generate statistical reports for groups of webforms"  
core            = "6.x"  
version         = "6.x-1.x-dev"  

dependencies[]  = webform, drupal_queue

The goal of this module is to automate the creation, storage, and display of reports for a group of webform along a pre-set interval. The use case it has been built for is that of a contest system, where a company runs multiple contests, and would like easily comparable data on the various contests.

###Design
The module allows the user to create "statistical groupings" of webforms. Each grouping has a set of metrics associated with it (measuring form component values). Groupings also have a reporting interval and may be generated according to a schedule. The reports are generated using the drupal_queue module, and therefore break reports up into multiple small queries that the queue can easily handle. In this way, the reports are generated in the background and will help to reduce php timeout errors for large dataset data exports.

###Issues
There are many issues to be tackled for this module (see the issue queue). Currently, the module can only count the number of occurances of a particular component data in the webform_submitted_data table, (sql COUNT() function). This needs to change. Also, select elements aren't handled well.
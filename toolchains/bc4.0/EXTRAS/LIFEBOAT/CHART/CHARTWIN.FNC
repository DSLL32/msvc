                    =============================
                     Index of Essential Graphics
                         Chart  for  Windows
                              Functions
                    =============================


FUNCTION                                         DESCRIPTION                                                           
========                          =========================================
AccessChart                       retrieve a chart attribute
BeginMessages			  start sending a block of chart messages
EndMessages                       end a block of messages
ChartMessage                      send a message to a chart
CreateChart                       create a chart
FreeChart                         remove a chart from the screen and free its memory
CreateChartMarker                 initialize a chart marker structure
CreateChartPattern                initialize a chart pattern structure
CreateLabelForChart               initialize a chart label structure
CreateSeriesLabel                 initialize a series label structure
DrawChart                         draw a chart
PrintChart                        print a chart
ReadChart                         read a chart into a window
WriteChart                        write a chart to disk

Pie and Pyramid 
Chart Messages                                  DESCRIPTIONS
===============                   =======================================
EGW_DATA                          pass the chart data                    
EGW_BACKGROUND                    select chart background color          
EGW_SLICE_LABEL                   Set up a slice label                   
EGW_SLICE_LABEL_POSITION          set slice label position               
EGW_VALUE_DISPLAY_TYPE            data label display type                
EGW_SIGNIFICANT_DIGITS            decimal places for data labels         
EGW_CHART_TITLE                   set the chart title                    
EGW_SLICE_PATTERN                 select slice pattern                   
EGW_BORDER_COLOR                  select slice border color              
EGW_BORDER_WEIGHT                 select slice border weight             
EGW_WINDOW                        resize chart to the full window        
EGW_WINDOW_AUTO_SCALE             automatically scale chart fonts       
EGW_DISPLAY_TYPE                  chart display method

Pyramid Specific Messages                                 DESCRIPTIONS
=========================               =======================================
EGW_PYRAMID_DATA_DISPLAY                select pyramid display type            
EGW_PYRAMID_DEPTH                       set pyramid depth                      

Pie Specific Messages                                     DESCRIPTIONS
=====================                   =======================================
EGW_EXPLODE_SLICE                       explode a pie slice                    
EGW_SLICE_HEIGHT                        select slice height                    
EGW_PIE_ROTATION                        set pie rotation angle                 
EGW_PIE_TILT                            set pie chart tilt                     
EGW_DATA_LABEL_POSITION                 set data label position                
EGW_DATA_LABEL_FONT                     select data label font                 
EGW_DATA_LABEL_COLOR                    select data label color                
                                        
Column Chart Messages                                     DESCRIPTIONS
=====================                   =======================================
EGW_SERIES_TYPE                         select column chart type               
EGW_DATA                                pass the chart data array              
EGW_BACKGROUND                          select chart background color          
EGW_DATA_AREA_BACKGROUND                select data area background            
EGW_SERIES_MARKER                       select line/scatter chart marker       
EGW_SERIES_PATTERN                      select series pattern type             
EGW_3D                                  turns 3-D option on/off                
EGW_SIGNIFICANT_DIGITS                  decimal places for float labels        
EGW_REVERSE                             draw chart upside down                
EGW_OVERLAP                             select bar chart overlap        
EGW_STACKED                             select a stacked bar chart        
EGW_HUNDRED_PERCENT                     select a hundred percent chart        
EGW_SERIES_TRANSPARENT_AREA             make a data series transparent        
EGW_SERIES_BORDER_COLOR                 select series border color             
EGW_SERIES_BORDER_STYLE                 select series border style             
EGW_SERIES_BORDER_WEIGHT                select series border weight            
EGW_ADD_SERIES_LABEL                    add a label to a data point            
EGW_DELETE_SERIES_LABEL                 remove a label from a data point       
EGW_WINDOW                              resize the chart to a full window      
EGW_WINDOW_AUTO_SCALE                   automatically scale chart fonts        
EGW_DISPLAY_TYPE                        chart display method
EGW_CATEGORY_SPACING                    set category bar spacing               
EGW_CATEGORY_LABEL                      select category label                  
EGW_CATEGORY_LABEL_ORIENTATION          set X-axis label orientation           
EGW_CATEGORY_LABEL_RATIO                vertical space for category labels     
EGW_CATEGORY_LABEL_POSITION             set category labels location           
EGW_CATEGORY_AXIS_TITLE                 set X-axis title                       
EGW_CATEGORY_AXIS_COLOR                 select X-axis color                    
EGW_CATEGORY_AXIS_WEIGHT                select the X-axis weight               
EGW_CATEGORY_AXIS_STYLE                 select the X-axis style                
EGW_CATEGORY_MAJOR_GRID                 display X-axis major grids             
EGW_CATEGORY_MINOR_GRID                 display X-axis minor grids             
EGW_CATEGORY_GRID_STYLE                 select X-axis grid style               
EGW_CATEGORY_GRID_COLOR                 set the X-axis grid color              
EGW_CATEGORY_GRID_WEIGHT                select the X-axis grid weight          
EGW_CATEGORY_MAJOR_TICK_TYPE            select the major X-axis tick type      
EGW_CATEGORY_MINOR_TICK_TYPE            select the minor X-axis tick type      
EGW_CATEGORY_LOG                        set X-axis to logarithmic scale        
EGW_CATEGORY_TICK_COLOR                 set X-axis tick color                  
                                        
Value (Y-axis) Messages                           DESCRIPTIONS                 
=======================                 =======================================
EGW_VALUE_MIN                           set min Y-axis value                   
EGW_VALUE_AUTO_MIN                      use auto Y-axis min value              
EGW_VALUE_MAX                           set max Y-axis value                   
EGW_VALUE_AUTO_MAX                      use auto Y-axis max value              
EGW_VALUE_MAJOR_INCREMENT               set Y-axis major increment             
EGW_VALUE_AUTO_MAJOR                    use auto Y-axis major increment        
EGW_VALUE_MINOR_INCREMENT               set Y-axis minor increment        
EGW_VALUE_AUTO_MINOR                    use auto Y-axis minor increment        
EGW_VALUE_AXIS_FONT                     select Y-axis font                     
EGW_VALUE_AXIS_LABEL_COLOR              select Y-axis label color              
EGW_VALUE_LABEL_ORIENTATION             set Y-axis label direction             
EGW_VALUE_LABEL_POSITION                set Y-axis label position              
EGW_VALUE_LABEL_RATIO                   available space for Y-axis labels      
EGW_VALUE_AXIS_TITLE                    set the Y-axis title                   
EGW_VALUE_TITLE_POSITION                set the Y-axis title position          
EGW_VALUE_AXIS_POSITION                 set Y-axis label location              
EGW_VALUE_AXIS_STYLE                    select Y-axis style                    
EGW_VALUE_AXIS_COLOR                    select Y-axis color                    
EGW_VALUE_AXIS_WEIGHT                   select Y-axis weight                   
EGW_VALUE_MINOR_GRID                    display Y-axis minor grids             
EGW_VALUE_MAJOR_GRID                    display Y-axis major grids             
EGW_VALUE_GRID_COLOR                    set Y-axis grid color                  
EGW_VALUE_GRID_STYLE                    select Y-axis grid style               
EGW_VALUE_GRID_WEIGHT                   select Y-axis grid weight              
EGW_VALUE_MAJOR_TICK_TYPE               select Y-axis major tick type          
EGW_VALUE_MINOR_TICK_TYPE               select Y-axis minor tick type          
EGW_VALUE_TICK_COLOR                    select Y-axis tick color               
EGW_VALUE_CROSSAT                       set X-axis cross value                 
EGW_VALUE_AUTO_CROSSAT                  use auto X-axis value crossing        
EGW_VALUE_LOG                           set Y-axis to logarithmic scale        

Series (Z-axis) Messages                          DESCRIPTIONS
=======================                 =======================================
EGW_SERIES_SPACING                      spacing between Z-axis series        
EGW_SERIES_AXIS_COLOR                   select Z-axis color                
EGW_SERIES_AXIS_STYLE                   select Z-axis style                
EGW_SERIES_AXIS_WEIGHT                  select Z-axis weight                
EGW_SERIES_LABEL                        set a series label                
EGW_SERIES_LABEL_ORIENTATION            set the series label direction        
EGW_SERIES_LABEL_POSITION               set the series label position         
EGW_SERIES_LABEL_COLOR                  set the Z-axis label color        
EGW_SERIES_LABEL_FONT                   set the Z-axis label font        
EGW_SERIES_MAJOR_GRID                   display Z-axis major grids        
EGW_SERIES_MINOR_GRID                   display Z-axis minor grids        
EGW_SERIES_GRID_COLOR                   set the color of the Z-axis grids
EGW_SERIES_GRID_STYLE                   select Z-axis grid style        
EGW_SERIES_GRID_WEIGHT                  select Z-axis grid weight         
EGW_SERIES_MAJOR_TICK_TYPE              select Z-axis major tick type        
EGW_SERIES_MINOR_TICK_TYPE              select the Z-axis minor tick type
EGW_SERIES_TICK_COLOR                   select Z-axis tick color        

3-D Column Chart Messages                         DESCRIPTIONS
=========================               =======================================
EGW_X_ROTATION                          set X-axis rotation                 
EGW_Y_ROTATION                          set Y-axis rotation                 
EGW_Z_ROTATION                          set Z-axis rotation                 
EGW_PERSPECTIVE                         set 3-D depth perspective        
EGW_BASE_COLOR                          select base color                 
EGW_DEPTH_WIDTH_RATIO                   set depth/width ratio                
EGW_HEIGHT_WIDTH_RATIO                  set height/width ratio                 

Bar Chart Messages                                DESCRIPTIONS
==================                      =======================================
EGW_HORIZONTAL_BAR                      select a horizontal bar chart        
EGW_3D_SLAP_ON                          select 3-D look for 2-D charts        
        
High-Low-Close Chart                              DESCRIPTIONS
====================                    =======================================
EGW_HIGH_LOW_LINE_COLOR                 set High-Low line color                
EGW_HIGH_LOW_LINE_WEIGHT                set High-Low line weight        
EGW_HIGH_COLOR                          set high tick color                 
EGW_LOW_COLOR                           set low tick color                 
EGW_CLOSE_COLOR                         set close tick color                
EGW_HIGH_TICK                           set high tick type                
EGW_LOW_TICK                            set low tick type                
EGW_CLOSE_TICK                          set close tick type                

Line Chart Messages                               DESCRIPTIONS
====================                    =======================================
EGW_LINE_CHART_LINE_COLOR               set the line color                 
EGW_LINE_CHART_LINE_STYLE               select the line style                 
EGW_LINE_CHART_LINE_WEIGHT              select the line weight                

Real Time Chart Messages                          DESCRIPTIONS
========================                =======================================
EGW_DATA_POINT                          pass a point to a realtime chart

Legend Messages                                   DESCRIPTIONS
========================                =======================================
EGW_LEGEND_VIEW                         select legend view                  
EGW_LEGEND_POSITION                     select legend position                
EGW_LEGEND_BACKGROUND_COLOR             select legend background color        
EGW_LEGEND_BORDER_COLOR                 select legend border color        
EGW_LEGEND_BORDER_WEIGHT                select the legend border weight        
EGW_LEGEND_FONT                         select the legend font                
EGW_LEGEND_TEXT_COLOR                   select legend text color        

Printer Messages                                  DESCRIPTIONS
========================                =======================================
EGW_DEFINE_PRINT_SIZE                   set user defined resolution         
EGW_PRINTER_RESOLUTION                  determine printer resolution        
EGW_PRINTER_MARGINS                     set printer margins                
EGW_PRINTER_AUTO_SCALE                  set automatic printer mode        
EGW_PRINTER_COPIES                      set copies to print                

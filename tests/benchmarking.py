#python

import testing
import k3d

CALC_AVERAGE = -1

polyGridRange = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]

def benchmark_path():
    return testing.benchmark_path()

# this class is used to setup an input mesh for mesh benchmarks
class benchmarkMesh(object):
    def __init__(self, counts = (1,1,1)):
        self.__counts = counts
        
    def set_document(self, document):
        self.__document = document
        self.initialize_mesh()
        
    def initialize_mesh(self):
        self.__cube = k3d.plugin.create("PolyCube", self.__document)
        self.__cube.columns = 1;
        self.__cube.rows = 1;
        self.__cube.slices = 1;
        self.__cube.width = 1;
        self.__cube.height = 1;
        self.__cube.depth = 1;
        
        self.__mesh_array_3D = k3d.plugin.create("MeshArray3D", self.__document)
        self.__mesh_array_3D.count1 = self.__counts[0]
        self.__mesh_array_3D.count2 = self.__counts[1]
        self.__mesh_array_3D.count3 = self.__counts[2]
    
        self.__mesh_array_3D.layout = k3d.plugin.create("TranslateArray3D", self.__document)
    
        k3d.property.connect(self.__document, self.__cube.get_property("output_mesh"), self.__mesh_array_3D.get_property("input_mesh"))
    
    def set_counts(self, counts):
        # counts is a tuple of values
        self.__counts = counts
        
    def get_mesh(self):
        return self.__mesh_array_3D
    
    def get_size_metric(self):
        return self.__counts[0]*self.__counts[1]*self.__counts[2]*4

# class to handle profiler output for benchmarking
class k3dProfilingProcessor(object):
    def __init__(self):
        self.__results = {}
        self.__results['Total'] = []
        self.__nodeName = ''
    
    def number_of_results(self):
        try:
            return len(self.__results['Total'])
        except:
            return 0
    
    def number_of_columns(self):
        return len(self.__results)
        
    def clear_results(self):
        self.__init__()
        
    def add_profiler_results_for_node(self, nodeName, profilerRecords):
        self.__nodeName = nodeName
        for (node, timing) in profilerRecords.items():
            if node.name == self.__nodeName:
                total = 0.0
                for t in timing:
                    total += timing[t]
                    if t in self.__results:
                        self.__results[t] += [timing[t],]
                    else:
                        self.__results[t] = [timing[t],]
                self.__results['Total'] += [total,]
                break
    
    # oputput the results as a table to display on the dashboard
    def output_as_dart_table(self, description = ''):
        if description:
            print '<DartMeasurement name="Description" type="text/string">' + description + '</DartMeasurement>'
    
        tmpString = '';
        headingOrder = []
        averages = []
        maximums = []
    
        for heading in self.__results:
            if heading != 'Total':
                tmpString += heading + ' : '
                headingOrder += [heading,]
            averages += [0.0,]
            maximums += [0.0,]
            
        tmpString += 'Total'
        headingOrder += ['Total',]
    
        print '<DartMeasurement name="HEADING" type="text/string">' + tmpString + '</DartMeasurement>'
        
        # get the number of self.__results
        numResults = self.number_of_results()
        numColumns = self.number_of_columns()
        
        for row in range(numResults):
            tmpString = ''
            for col_index in range(numColumns):
                heading = headingOrder[col_index]
                # update the averages and the maximums
                averages[col_index] += self.__results[heading][row]
                if self.__results[heading][row] > maximums[col_index]:
                    maximums[col_index] = self.__results[heading][row]
                
                tmpString += str(self.__results[heading][row])
                if heading != 'Total':
                    tmpString += ' : '
    
            print '<DartMeasurement name="run' + str(row) + '" type="text/string">' + tmpString + '</DartMeasurement>'
            
        # now handle the averages and the maximums
        tmpString = ''
        tmpStringMax = ''
        for col_index in range(numColumns):
            tmpString += str(averages[col_index]/numResults)
            tmpStringMax += str(maximums[col_index])
            if col_index < ( numColumns - 1 ):
                tmpString += ' : '
                tmpStringMax += ' : '
        
        print '<DartMeasurement name="Average" type="text/string">' + tmpString + '</DartMeasurement>'
        print '<DartMeasurement name="Maximums" type="text/string">' + tmpStringMax + '</DartMeasurement>'
    
    # output the results to a comma-separated file
    def output_as_CSV_file(self, filename, description = '', appendFile = False):
        def output_headings(file):
            tmpString = '';
            headingOrder = []
    
            for heading in self.__results:
                if heading != 'Total':
                    tmpString += '"' + heading + '",'
                    headingOrder += [heading,]
                
            tmpString += '"Total"'
            headingOrder += ['Total',]
        
            file.write(tmpString + '\n')
            return headingOrder
        
        def output_result_data(file, headingOrder):
            numResults = self.number_of_results()
            numColumns = self.number_of_columns()
            
            for row in range(numResults):
                tmpString = ''
                for col_index in range(numColumns):
                    heading = headingOrder[col_index]
                    
                    tmpString += str(self.__results[heading][row])
                    if heading != 'Total':
                        tmpString += ','
        
                file.write(tmpString + '\n')
        
        if appendFile:
            out_file = open(filename, 'a+')
        else:
            out_file = open(filename, 'w')
            
        try:
            if description:
                out_file.write('"' + self.__nodeName + '",'  +  description + ',' + str(self.number_of_results()) + '\n')
            headingOrder = output_headings(out_file)
            output_result_data(out_file, headingOrder)
        finally:
              out_file.close()
       
    def output_file_to_Dart(self, filename):
         print """<DartMeasurementFile name="CSV_file" type="text/plain">""" + str(filename) + """</DartMeasurementFile>"""
    
    def output_and_save(self, nodeName, description, appendToFile, firstInFile):
        self.output_as_dart_table(nodeName + ' : ' + description)
        # save to CSV file
        if (appendToFile or firstInFile):
            if firstInFile:
                appendToFile = False
            CSV_output_file = k3d.filesystem.generic_path(benchmark_path() + '/' + nodeName + '.benchmark.txt')
        else:
            CSV_output_file = k3d.filesystem.generic_path(benchmark_path() + '/' + nodeName + description + '.benchmark.txt')
        
        self.output_as_CSV_file(str(CSV_output_file), description, appendToFile)

def total_profiler_time(profilerRecords):
  total = 0.0
  for (node, timing) in profilerRecords.items():
    for t in timing:
        total += timing[t]
  return total

def print_profiler_records(profilerRecords):
  for (node, timing) in profilerRecords.items():
    for t in timing:
        print """<DartMeasurement name=\"""" + node.name + "::" + t +  """\" type="numeric/float">""" + str(timing[t]) + """</DartMeasurement>"""
        
class ResultSet(object):
    def __init__(self, x, y, label, plot_style):
        self.x = x
        self.y = y
        self.label = label
        self.plot_style = plot_style
    
class ResultsProcessor(object):
    def __init__(self, operation = CALC_AVERAGE):
        self.__operation = operation
        self.__results = []
    
    def add_dataset(self, benchmarkPluginName, ColumnTitle, plot_style = 'o-'):
        print "Adding Dataset %s : %s" % (benchmarkPluginName, ColumnTitle)
        (x, y) = extract_data(benchmarkPluginName, ColumnTitle, self.__operation)
        self.__results += [ResultSet(x, y, (benchmarkPluginName, ColumnTitle, self.__operation), plot_style)]
        
    def plot_data(self, xlabel=None, ylabel=None):
        try:
            import pylab as P
        except:
            return 0
        
        legend = ()
        for result in self.__results:
            P.plot(result.x, result.y, result.plot_style)
            legend += ("%s : %s" % (result.label[0], result.label[1]),)
        
        
        if xlabel:
            P.xlabel(xlabel)
        if ylabel:
            P.ylabel(ylabel)
        
        P.legend(legend, loc='upper left')
        # set the legend fontsize    
        ltexts = P.gca().get_legend().get_texts()
        for lt in ltexts:
            P.setp(lt, fontsize = 10)
        
        return 1
            
    def plot_speedup(self, xlabel=None, ylabel=None, ref_index = 0):
        try:
            import pylab as P
        except:
            return 0
        
        x = self.__results[0].x 
        
        y_ref = self.__results[0].y
        
        P.plot([x[0], x[-1]], [1,1], 'k--');
        legend = ('Unity',)
        
        for r_index in range(1,len(self.__results)):
            speedup = []
            for k in range(len(self.__results[r_index].y)):
                y = self.__results[r_index].y[k]
                try:
                    speedup += [y_ref[k]/y, ]
                except:
                    speedup += [0, ]
                
            P.plot(x, speedup, self.__results[r_index].plot_style)
            legend += ("%s : %s" % (self.__results[r_index].label[0], self.__results[r_index].label[1]),)
            
        
        if xlabel:
            P.xlabel(xlabel)
        
        P.ylabel('Speedup vs %s:%s' % (self.__results[0].label[0], self.__results[0].label[1]))
        
        P.legend(legend, loc='lower right')
        # set the legend fontsize    
        ltexts = P.gca().get_legend().get_texts()
        for lt in ltexts:
            P.setp(lt, fontsize = 10)
        
        return 1
        
# run a mesh modifier benchmark for the specified node
def mesh_modifier_benchmark(benchmarkPluginName, maxSize = 15, properties = {"input_matrix" : k3d.translate3(k3d.vector3(0, 0, 1))}):
    current_count = [1,1,1]
    runsPerBenchmark = 10
    append = True
    for k in range(maxSize):
        try:
            benchmark_mesh = benchmarkMesh(current_count)
            run_mesh_modifier_benchmark(benchmarkPluginName, benchmark_mesh, runsPerBenchmark, properties, append, k == 0)
            current_count[k % 3] *= 2
        except:
            break
        
def mesh_source_benchmark(benchmarkPluginName, properties = {}):
    num_runs = 10e10
    for p in properties.values():
        if len(p) < num_runs:
            num_runs = len(p)
    
    numberOfRuns = 10   
    for k in range(num_runs):
        properties_for_run = {}
        for (p, val) in properties.items():
            properties_for_run[p] = val[k]
            
        run_mesh_source_benchmark(benchmarkPluginName, numberOfRuns, properties_for_run, True, k == 0)
    

def run_mesh_source_benchmark(meshSourceNodeName, numberOfRuns = 1, properties = {}, appendToFile = False, firstInFile=False):
    document = k3d.new_document()
        
    profiler = k3d.plugin.create("PipelineProfiler", document)
    benchmarkNode = k3d.plugin.create(meshSourceNodeName, document)
    
    sizeMetric = 1
    for (p, val) in properties.items():
        benchmarkNode.get_property(p).set_value(val)
        sizeMetric *= val
   
    profilingResults = k3dProfilingProcessor()
    for n in range(numberOfRuns):
        benchmarkNode.output_mesh
        profilingResults.add_profiler_results_for_node(meshSourceNodeName, profiler.records)
    
    profilingResults.output_and_save(meshSourceNodeName, '%d' % (sizeMetric), appendToFile, firstInFile)
 
        
def convert_dim_string_to_size_measure(dimString):
    # image where dimension is given as (width)x(height)
    if 'x' in dimString:
        dims = dimString.split('x')
        return int(dims[0])*int(dims[1])
    
    # as default try to convert string to int and return
    # return 0 if it fails
    try:
        dims = int(dimString)
        return dims
    except:
        pass

    return 0
 
 
"""
    Read the data from a benchmark file generated py parsing the pipeline profiler output
    @param - bencmarkPluginName : the name of the plugin that was benchmarked
    @param - ColumnTitle : the column of interest
    @param - operation : specify the operation to perform on the data.  If operation >= 0 then a specific run is 
                         selected.
"""        
def extract_data(benchmarkPluginName, ColumnTitle = "Total", operation = CALC_AVERAGE):
    """
        Open the benchmark file and read the lines
    """
    def read_benchmark_file(pluginName):
        benchmarkFilename = benchmark_path() + '/' + benchmarkPluginName + '.benchmark.txt'
        f = open(benchmarkFilename, 'r')
        lines = f.readlines()
        f.close()
        return (lines, len(lines))
    
    def process_header_lines(lines, k):
        splitLine = lines[k].strip().split(',')
        dimension = convert_dim_string_to_size_measure(splitLine[1])
        number_of_runs = int(splitLine[2])
        column_of_interest_index = -1;
        splitLine = lines[k+1].strip().split(',')
        if ('"' + ColumnTitle + '"') in splitLine:
            column_of_interest_index = splitLine.index('"' + ColumnTitle + '"')
            
        return (k+2, dimension, number_of_runs, column_of_interest_index)
    
    def process_data_lines(lines, k, number_of_runs, column_of_interest_index, operation):    
        """
            Retrive the desired data from the lines
            @param operation - specify the row of interest or average, max, min
        """
        if operation > 0:
            splitLine = lines[k+operation-1].strip().split(',')
            datum = float(splitLine[column_of_interest_index])
        elif operation == CALC_AVERAGE:
            datum = 0
            for data_row in range(k, k+number_of_runs):
                splitLine = lines[data_row].strip().split(',')
                datum += float(splitLine[column_of_interest_index])
            datum /= number_of_runs
        else:
            datum = -1
        
        return (k+number_of_runs, datum)
    
    try:    
        (lines, number_of_lines) = read_benchmark_file(benchmarkPluginName)
    except:
        print "Error reading file"
        return (0, 0)
    
    k = 0;
    data_list = []
    dimension_list = []
    
    while k < number_of_lines:
        (k, dimension, number_of_runs, column_of_interest_index) = process_header_lines(lines, k)
        if k >= number_of_lines:
            break
        (k, datum) = process_data_lines(lines, k, number_of_runs, column_of_interest_index, operation)
        dimension_list += [dimension, ]
        data_list += [datum, ]
    
    return (dimension_list, data_list)
        
    

"""
    Run a benchmark for the specified mesh modifier
"""
def run_mesh_modifier_benchmark(meshModifierNodeName, benchmarkMesh, numberOfRuns = 1, properties = {}, appendToFile = False, firstInFile=False):
    document = k3d.new_document()
        
    profiler = k3d.plugin.create("PipelineProfiler", document)
    
    benchmarkMesh.set_document(document)
    
    inputNode = benchmarkMesh.get_mesh()
    
    selection = k3d.select_all()
    

    benchmarkNode = k3d.plugin.create(meshModifierNodeName, document)
    for (p, val) in properties.items():
        benchmarkNode.get_property(p).set_value(val)
    benchmarkNode.mesh_selection = selection
        
    profilingResults = k3dProfilingProcessor()
    for n in range(numberOfRuns):
        k3d.property.connect(document, inputNode.get_property("output_mesh"), benchmarkNode.get_property("input_mesh"))
        benchmarkNode.output_mesh
        profilingResults.add_profiler_results_for_node(meshModifierNodeName, profiler.records)
    
    description = '%d' % (benchmarkMesh.get_size_metric())
    profilingResults.output_and_save(meshModifierNodeName, description, appendToFile, firstInFile)

"""
    selected is a list of tuples (PluginName, ColumnName)
"""
def compare_and_output_image(filename, selected_benchmarks, plotLabels = (None, None), plotSpeedup = False):
    
    try:
        import pylab as P
    except:
        print "Pylab not found"
        return 0;
    
    import os
    
    processor = ResultsProcessor()
    
    for selected in selected_benchmarks:
        processor.add_dataset(selected[0], selected[1])
    
    if plotSpeedup:
        if processor.plot_speedup(plotLabels[0], plotLabels[1]) != 0:
            speedup_filename =  benchmark_path() + '/' + filename + '.speedup.png';
            try:
                os.delete(speedup_filename)
            except:
                pass
            
            P.savefig(speedup_filename, format='png')
            
            P.close()
        
            return speedup_filename
        
    if processor.plot_data(plotLabels[0], plotLabels[1]) != 0:
        image_filename = benchmark_path() + '/' + filename + '.png';
        try:
            os.delete(image_filename)
        except:
            pass
        
        P.savefig(image_filename, format='png')
        
        P.close()
        
        return image_filename
    
    return None
    
    
def generate_component_image(description, node_name, columns = ["Total"] ):
    selected = []
    for col in columns:
        selected += [(node_name, col),]
        
    filename = compare_and_output_image(description, selected)
    
    if filename != 0:
        print '<DartMeasurementFile name="' + description + '" type="image/png">' + str(filename) + '</DartMeasurementFile>'
    else:
        print '<DartMeasurement name="' + description + '" type="text/string">  Error in comparison </DartMeasurementFile>'

def generate_comparison_image(description, run_names, column = "Total", plotLabels = (None, None), calculate_speedup = False):
    selected = []
    for run in run_names:
        selected += [(run, column),]
    
    filename = compare_and_output_image(description, selected, plotLabels)
    
    
    if filename:
        print '<DartMeasurementFile name="' + description + '" type="image/png">' + str(filename) + '</DartMeasurementFile>'
    else:
        print '<DartMeasurement name="' + description + '" type="text/string">  Error in comparison </DartMeasurementFile>'
    
    if calculate_speedup:
        filename = compare_and_output_image(description, selected, plotLabels, plotSpeedup = True)
        if filename:
            print '<DartMeasurementFile name="' + description + 'Speedup" type="image/png">' + str(filename) + '</DartMeasurementFile>'
        else:
            print '<DartMeasurement name="' + description + 'Speedup" type="text/string">  Error in comparison </DartMeasurementFile>'

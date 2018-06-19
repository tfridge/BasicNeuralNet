
//
// SimpleNeuralDataCreator.cpp : Defines the entry point for the console application.

/**************************************************************/
//			Simple Data Creation Tool that makes
//			training data for a Neural Net
//			This data is just an XOR bitwise operator
//			Written by David Miller 
//
//
//			BasicNeuralNet.cpp : 
//			Defines the entry point for the console application.
//
//			To Build make sure valid data is in directory
//			Requires "topology:" and at least three layers
//			Requires "in:" for input and "out:" for output
//			
//
//			From GitBash, run:  
//			 ./BasicNeuralNet.exe > out.txt
//
/**************************************************************/

#include "stdafx.h"
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <tuple>

class Neuron;
using std::vector;
using std::cout;

//Simple tuple for holding the value of a columns's respective average and range.
typedef std::tuple<double, double> statpair;

//Simple trining data class to get input data into the Neural Net
class TrainingData
{
public:
	TrainingData(const std::string filename);
	bool isEof(void) { return m_trainingDataFile.eof(); }
	void getTopology(vector<unsigned int> &topology);

	unsigned int getNextInputs(vector<double> &inputVals);
	unsigned int getNextFireInputs(vector<double>& inputVals);
	unsigned int getTargetOutputs(vector<double> &targetOutputVals);

	//The following methods/members are for storing statistical information for normalizing inputs

	void normalizeData();
	vector<statpair> normalizedInput;
	vector<statpair> normalizedOutput;
	vector<vector<double>> fullInput;
	vector<vector<double>> fullOutput;

	vector<statpair> getInputStatpair() { return normalizedInput; }
	vector<statpair> getOutputStatpair() { return normalizedOutput; }

private:
	std::ifstream m_trainingDataFile;
};

void TrainingData::normalizeData() 
{
	for (vector<vector<double>>::iterator index = fullInput.begin(); index != fullInput.end(); index++)
	{
		double range;
		double min = 10000;
		double max = 0;
		for (vector<double>::iterator curInput = index->begin(); curInput != index->end(); curInput++) 
		{
			if (*curInput > max) {
				max = *curInput;
			}
			if (*curInput < min) {
				min = *curInput;
			}
		}
		range = max - min;
		normalizedInput.push_back(statpair(min, range));
	}

	for (vector<vector<double>>::iterator index = fullOutput.begin(); index != fullOutput.end(); index++)
	{
		double range;
		double min = 10000;
		double max = 0;
		for (vector<double>::iterator curOutput = index->begin(); curOutput != index->end(); curOutput++) 
		{
			if (*curOutput > max) {
				max = *curOutput;
			}
			if (*curOutput < min) {
				min = *curOutput;
			}
		}
		range = max - min;
		normalizedOutput.push_back(statpair(min, range));
	}
}

//Unchanged between basic and fire sets
void TrainingData::getTopology(vector<unsigned int> &topology)
{
	std::string line;
	std::string label;

	std::getline(m_trainingDataFile, line);
	std::stringstream ss(line);

	ss >> label;

	cout << label;

	if (this->isEof() || label.compare("topology:") != 0)
	{
		abort();
	}

	while (!ss.eof())
	{
		unsigned int n;
		ss >> n;
		topology.push_back(n);
	}
	fullInput.resize(topology[0]);
	fullOutput.resize(topology[2]);
	return;
}

//Unchanged between basic and fire sets
TrainingData::TrainingData(const std::string filename)
{
	m_trainingDataFile.open(filename.c_str());
}

//Unchanged between basic and fire sets
unsigned int TrainingData::getNextInputs(vector<double> &inputVals)
{
	inputVals.clear();

	std::string line;
	std::getline(m_trainingDataFile, line);
	std::stringstream ss(line);

	std::string label;
	ss >> label;
	if (label.compare("in:") == 0)
	{
		double oneValue;
		while (ss >> oneValue)
		{
			inputVals.push_back(oneValue);
		}
	}
	return inputVals.size();
}

//Variation of the above method to handle taking
unsigned int TrainingData::getNextFireInputs(vector<double> &inputVals)
{
	inputVals.clear();

	std::string line;
	std::getline(m_trainingDataFile, line);
	std::stringstream ss(line);

	std::string label;
	ss >> label;
	if (label.compare("in:") == 0)
	{
		double oneValue;
		unsigned int count = 0;
		while (ss >> oneValue)
		{
			inputVals.push_back(oneValue);
			fullInput[count].push_back(oneValue);
			count++;
		}
	}
	return inputVals.size();
}

//Unchanged between basic and fire sets
unsigned int TrainingData::getTargetOutputs(vector<double> &targetOutputVals)
{
	targetOutputVals.clear();

	std::string line;

	getline(m_trainingDataFile, line);
	std::stringstream ss(line);

	std::string label;

	ss >> label;

	if (label.compare("out:") == 0)
	{
		double oneValue;
		unsigned int count = 0;
		while (ss >> oneValue)
		{
			targetOutputVals.push_back(oneValue);
			fullOutput[count].push_back(oneValue);
			count++;
		}
	}
	return targetOutputVals.size();
}



struct Connection
{
	double weight;

	double deltaWeight;

};

typedef vector<Neuron> Layer;


// ***********************class Neuron *********************/

class Neuron
{
public:
	Neuron(unsigned int numOutputs, unsigned int myIndex);
	void setOutputVal(double val) { m_outputVal = val; }
	double getOutputVal(void) const { return m_outputVal; }
	void feedForward(const Layer &prevLayer);
	void calcOutputGradients(double targetVal);
	void calcHiddenGradients(const Layer &nextLayer);
	void updateInputWeights(Layer &prevLayer);

private:
	static double eta; //[0.0 ... overall net training rate
	static double alpha; //[0.0 ... n] multiplier of last weight change (momentum)

	static double transferFunction(double x);
	static double transferFunctionDerivative(double x);

	static double randomWeight(void) { return rand() / double(RAND_MAX); }
	double SumDOW(const Layer &nextLayer) const;

	double m_outputVal;
	vector<Connection> m_outputWeights;
	unsigned int m_myIndex;
	double m_gradient;
};

double Neuron::eta = 0.15; //overall learning rate [0.0 .. 1.0] (0 slow learner, 1 fast learner)
double Neuron::alpha = .5; // Momentum, multiplier of last deltaWeight [0.0 ... 1.0]

void Neuron::updateInputWeights(Layer &prevLayer)
{
	//The weights to be updated are in the Connection container
	//in the neurons in the preceding layer

	for (unsigned n = 0; n < prevLayer.size(); ++n)
	{
		Neuron &neuron = prevLayer[n];

		double oldDeltaWeight = neuron.m_outputWeights[m_myIndex].deltaWeight;

		double newDeltaWeight =
			//Individual input, magnified by the gradient and train rate

			eta //overall learning rate
			* neuron.getOutputVal() //neuron_getOutputVal()
			* m_gradient
			//also add momentum = a fraction of the previous delta weight
			+ alpha
			* oldDeltaWeight;

		neuron.m_outputWeights[m_myIndex].deltaWeight = newDeltaWeight;
		neuron.m_outputWeights[m_myIndex].weight += newDeltaWeight;
	}
}

double Neuron::SumDOW(const Layer &nextLayer) const
{
	double sum = 0.0;

	//sum our contributions of the error at the nodes we feed
	for (unsigned n = 0; n < nextLayer.size() - 1; ++n)
	{
		sum += m_outputWeights[n].weight * nextLayer[n].m_gradient;
	}
	return sum;
}

void Neuron::calcHiddenGradients(const Layer &nextLayer)
{
	double dow = SumDOW(nextLayer);
	m_gradient = dow * Neuron::transferFunctionDerivative(m_outputVal);
}

void Neuron::calcOutputGradients(double targetVal)
{
	double delta = targetVal - m_outputVal;
	m_gradient = delta * Neuron::transferFunctionDerivative(m_outputVal);
}

double Neuron::transferFunction(double x)
{
	//tanh  output range [-1.0 ... 1.0]
	return tanh(x);
}

double Neuron::transferFunctionDerivative(double x)
{
	//tanh  derivative
	return 1.0 - x * x;
}

void Neuron::feedForward(const Layer &prevLayer)
{
	double sum = 0.0;

	//sum the previous layer's outputs (which are out inputs)
	//Include the bias node from the previous layer

	for (unsigned int n = 0; n < prevLayer.size(); ++n)
	{
		sum += prevLayer[n].getOutputVal() * prevLayer[n].m_outputWeights[m_myIndex].weight;
	}

	m_outputVal = Neuron::transferFunction(sum);
}

Neuron::Neuron(unsigned int numOutputs, unsigned int myIndex)
{
	for (unsigned int c = 0; c < numOutputs; ++c)
	{
		m_outputWeights.push_back(Connection());
		m_outputWeights.back().weight = randomWeight();


	}
	m_myIndex = myIndex;
}

// ***********************class Net *********************/
class Net
{
public:
	Net(const vector<unsigned int> &topology);
	void feedForward(const vector<double> &inputVals);
	void backProp(const vector <double> &targetVals);
	void getResults(vector<double> &resultVals) const;
	double getRecentAverageError(void) const { return m_recentAverageError; }

private:
	vector<Layer> m_layers; //m_layers[layerNum][neuronNum]
	double m_error;
	double m_recentAverageError;
	static double m_recentAverageSmoothingFactor;

};

double Net::m_recentAverageSmoothingFactor = 100.0; // Number of training samples to average over

void Net::getResults(vector<double> &resultVals) const
{
	resultVals.clear();

	for (unsigned int n = 0; n < m_layers.back().size() - 1; ++n)
	{
		resultVals.push_back(m_layers.back()[n].getOutputVal());
	}
}

void Net::backProp(const vector<double> &targetVals)
{
	//Calculate overall net error (RMS of output neuron errors)
	Layer &outputLayer = m_layers.back();
	m_error = 0.0;

	for (unsigned int n = 0; n < outputLayer.size() - 1; ++n)
	{
		double delta = targetVals[n] - outputLayer[n].getOutputVal();
		m_error += delta * delta;
	}

	m_error /= outputLayer.size() - 1; // get average error squared
	m_error = sqrt(m_error);  //RMS

	//implement a recent average measurment:
	m_recentAverageError =
		(m_recentAverageError * m_recentAverageSmoothingFactor + m_error)
		/ (m_recentAverageSmoothingFactor + 1.0);

	//Calculate output layer gradients
	for (unsigned int n = 0; n < outputLayer.size() - 1; ++n)
	{
		outputLayer[n].calcOutputGradients(targetVals[n]);
	}
	//Calculate gradients on hidden layers
	for (unsigned int layerNum = m_layers.size() - 2; layerNum > 0; --layerNum)
	{
		Layer &hiddenLayer = m_layers[layerNum];
		Layer &nextLayer = m_layers[layerNum + 1];

		for (unsigned n = 0; n < hiddenLayer.size(); ++n)
		{
			hiddenLayer[n].calcHiddenGradients(nextLayer);
		}
	}

	//For all layers from outputs to first hidden layer,
	//update connection weights
	for (unsigned layerNum = m_layers.size() - 1; layerNum > 0; --layerNum)
	{
		Layer &layer = m_layers[layerNum];
		Layer &prevLayer = m_layers[layerNum - 1];

		for (unsigned int n = 0; n < layer.size() - 1; ++n)
		{
			layer[n].updateInputWeights(prevLayer);
		}
	}
}

void Net::feedForward(const vector<double> &inputVals)
{
	assert(inputVals.size() == m_layers[0].size() - 1);

	//Assign (latch) the input values into the input neurons
	for (unsigned i = 0; i < inputVals.size(); ++i)
	{
		m_layers[0][i].setOutputVal(inputVals[i]);
	}

	//Forward propagate
	for (unsigned int layerNum = 1; layerNum < m_layers.size(); ++layerNum)
	{
		Layer &prevLayer = m_layers[layerNum - 1];

		for (unsigned int n = 0; n < m_layers[layerNum].size() - 1; ++n)
		{
			m_layers[layerNum][n].feedForward(prevLayer);
		}
	}

}

Net::Net(const vector<unsigned int> &topology)
{
	unsigned int numLayers = topology.size();
	for (unsigned int layerNum = 0; layerNum < numLayers; ++layerNum)
	{
		m_layers.push_back(Layer());
		unsigned numOutputs = layerNum == topology.size() - 1 ? 0 : topology[layerNum + 1];
		//We have made a new Layer, now fill in ith neurons, and
		//add a bias neuron to the layer less than equal

		for (unsigned int neuronNum = 0; neuronNum <= topology[layerNum]; neuronNum++)
		{

			//back gives you the last element in the container
			m_layers.back().push_back(Neuron(numOutputs, neuronNum));

			cout << "Made a Neuron!" << std::endl;

		}

		//force the bias node's output value to 1.0.  Its the last neuron created
		m_layers.back().back().setOutputVal(1.0);
	}

}


void showVectorVals(std::string label, vector<double> &v)
{
	cout << label << " ";
	for (unsigned int i = 0; i < v.size(); ++i)
	{
		cout << v[i] << " ";
	}

}

//
// THIS MAIN FUNCTION HAS BEEN UPDATED. It now handles the fire data.
//
int main()
{
	TrainingData trainData("tmp/trainingData.txt");
	TrainingData setupData("tmp/trainingData.txt");
	
	//Eg (3,2,1)
	vector<unsigned int> topology;
	vector<unsigned int> setupTopology;

	trainData.getTopology(topology);
	setupData.getTopology(setupTopology);

	Net myNet(topology);


	vector<double> inputVals, targetVals, resultVals;
	int trainingPass = 0;

	vector<statpair> inputNormalizer;
	vector<statpair> outputNormalizer;

	while (!setupData.isEof())
	{
		if (setupData.getNextFireInputs(inputVals) != topology[0]) {
			break;
		}
		setupData.getTargetOutputs(targetVals);
	}

	setupData.normalizeData();
	inputNormalizer = setupData.getInputStatpair();
	outputNormalizer = setupData.getOutputStatpair();

	int correct = 0;
	while (!trainData.isEof())
	{
		++trainingPass;

		cout << std::endl << "Pass " << trainingPass;

		if (trainData.getNextFireInputs(inputVals) != topology[0])
		{
			break;
		}
		if (inputVals.size() == inputNormalizer.size()) {
			int count = 0;
			for (vector<double>::iterator currentVal = inputVals.begin(); currentVal != inputVals.end(); currentVal++) {
				*currentVal = (*currentVal - std::get<0>(inputNormalizer[count]))/(std::get<1>(inputNormalizer[count]));
				count++;
			}
		}
		showVectorVals(": Inputs:", inputVals);
		cout << std::endl;
		myNet.feedForward(inputVals);

		//Collect the net's actual results
		myNet.getResults(resultVals);
		showVectorVals("Outputs:", resultVals);

		//train the net what the outputs should have been:
		trainData.getTargetOutputs(targetVals);
		if (targetVals.size() == outputNormalizer.size()) {
			int count = 0;
			for (vector<double>::iterator currentVal = targetVals.begin(); currentVal != targetVals.end(); currentVal++) {
				*currentVal = (*currentVal - std::get<0>(outputNormalizer[count])) / (std::get<1>(outputNormalizer[count]));
				count++;
			}
		}
		showVectorVals("Targets:", targetVals);
		cout << std::endl;

		assert(targetVals.size() == topology.back());

		myNet.backProp(targetVals);

		//Report how well the trainin is working averaged over recent
		cout << "Net recent average error: " << myNet.getRecentAverageError() << std::endl;

		//For debugging purposes only - remove in the future!
		if (resultVals[0] > 0 && targetVals[0] != 0) {
			correct++;
		}
		else if (resultVals[0] < 0 && targetVals[0] == 0) {
			correct++;
		}
	}
	cout << std::endl << "Done" << std::endl;

	std::string mystr;
	cout << correct << " fires were 'accurately' predicted out of " << trainingPass << ".";
	std::getline(std::cin, mystr);

	return 0;
}




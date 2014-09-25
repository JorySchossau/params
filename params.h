/* Copyright (c) 2014, Jory Schossau
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Description
// An arguments (flags / options) parser for c++, mostly to solve my own requirements and annoyances.
// Given some information about variables you have set aside to contain arguments data,
// this tool will parse argv and populate the variables. It will also handle cases where
// data was required, or defaults were used, or multiple values to read were necessary.
//
// Features
// * Easy to use yet powerful
// * Only one file to include
// * Written in C++11 for extra clarity
// * Produces help-style parameter details as a string for inclusion (string argdetails())
// * Enforces help messages for each parameter (no lazy programming!)
// 
// To Do
// * Check for option aliasing
//
// Usage
// simply include this header file in your c++ project. #include "params.h"
// How it works is you must declare all the variables which represent flags or parameters
// then gives those addresses when you create parameters using this tool.
// Then call argparse(argv);
// Those variables now contain the command line data.
// argdetails() returns a string of formatted help-style details of all your parameters for inclusion in help output.
//
// Notes
// * "--seed" options don't require dashes. Could use "-s" or "seed" or "s" etc.
// * Supports equals sign or space: 1) --seed 3   2) --seed=3
// * A TYPE::BOOL will be true or false depending if it exists, such as "--help" would be a good use.
// * Strings with spaces require escaped quotes (to prevent shell expansion): --username \"Jory Schossau\"
// * Default values are specified as a string before the option in addp: "3.14" or "-9" or "User" for example.
// * Options which need more than 1 argument may be specified as an integer after the variable:
// *   addp(TYPE::INT, &quantity, 3, "--quantity", "The quantity of materials to simulate.");
// *   would require an invocation option like this: --quantity 88 28 53
// *   and it will give an error for != 3 arguments specified.
// *   Requires the variable to be a vector.
// * Options which need infinite arguments (such as a list of files) are specified as a -1:
// *   addp(TYPE::INT, &quantity, -1, "--quantity", "The quantities to use of n items.");
// *   would require an invocation option like this: --quantity 17 16 62 21 31 42 98 34 52
// * To make an option not required, give it a default value, or specify 'false' for required. BOOL cannot be required.
// * Print help details for parameters with argdetails(), such as:
// *   cout << argdetails() << endl;
//
// Example:
/*
-- file main.cpp --
#include "params.h"
using namespace Params; // makes life a little easier, but might pollute namespace
int main(int argc, char* argv[]) {
	bool showhelp;
	int iterations;
	vector<float> seeds;
	string name_of_run;
	addp(TYPE::INT, &iterations, "--iterations", "The number of iterations to perform."); // minimum required signature
	addp(TYPE::FLOAT, &seeds, 3, false, "--seeds", "The seeds to begin simulation."); // 3 floats, not required
	addp(TYPE::STRING, &name_of_run, "simulation", "--name", "The name for this simulation run."); // defaults to "simulation"
	addp(TYPE::BOOL, &showhelp, "--help", "Shows this help message.");
	argparse(argv); // initiate parsing of arguments
	if (showhelp) {
		printf("%s\n", argdetails().c_str());
		exit(0);
	}
	// simulation code here
	exit(0);
}
*/

#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <initializer_list>
#include <cassert>
#include <vector>
#include <locale>
#include <stdexcept>

namespace Params {
	using namespace std;

	enum class TYPE {BOOL, INT, UINT, FLOAT, LONG, DOUBLE, CHAR, STRING};
	enum class OPT : int {XARGS=1, REQUIRED=2}; // just be powers of 2

	namespace priv {
		class Param {
			private:
			public:
				TYPE type;
				void* destination;
				string longPhrase;
				string helpPhrase;
				int xargs;
				bool required;
				bool set; // if required then should be set by end of arparse (we check)
				string defaultValue;
				Param(TYPE _type, void* _destination, string _longPhrase, string _helpPhrase, int _xargs=1, bool _required=true, string _defaultValue="");
				void Set(string _value);
		};

		static map<string, Param*> params_map;

		Param::Param(TYPE _type, void* _destination, string _longPhrase, string _helpPhrase, int _xargs, bool _required, string _defaultValue) : type(_type), destination(_destination), longPhrase(_longPhrase), helpPhrase(_helpPhrase), xargs(_xargs), required(_required), defaultValue(_defaultValue) {
			if (type == TYPE::BOOL) {
				set = true;
				required = false;
				if (defaultValue != "") {
					Set(defaultValue);
				} else {
					Set("false");
				}
			} else {
				if (xargs == 1) {
					Set(defaultValue);
				}
			} 
		}

		void Param::Set(string value) {
			if (value == "")
				return;
			switch(type) {
				case TYPE::BOOL: {
										  bool* variable = static_cast<bool*>(destination);
										  string lowercase="";
										  for (auto ch : value) { lowercase.push_back( char(tolower(ch)) ); }
										  if (lowercase != "false" && lowercase != "true") {
											  fprintf(stderr, "Unrecognized default value for boolean option: '%s'\n", value.c_str());
											  exit(1);
										  }
										  if (lowercase == "true") {
											  *variable = true;
										  } else if (lowercase == "false") {
											  *variable = false;
										  }
									  }
									  break;
				case TYPE::INT: {
										 if (xargs == 1) {
											 int* variable = static_cast<int*>(destination);
											 *variable = stoi(value);
										 } else {
											 vector<int>* variable = static_cast<vector<int>*>(destination);
											 try {
											 variable->push_back(stoi(value));
											 } catch ( ... ) {
												 fprintf(stderr, "Error in argument (expected type INT): %s\n", value.c_str());
												 fprintf(stderr, "Options which expect infinite arguments should be last.\n");
												 exit(1);
											 }
										 }
									 }
									 break;
				case TYPE::FLOAT: {
											if (xargs == 1) {
												float* variable = static_cast<float*>(destination);
												*variable = stof(value);
											} else {
												vector<float>* variable = static_cast<vector<float>*>(destination);
												try {
												variable->push_back(stof(value));
											 } catch ( ... ) {
												 fprintf(stderr, "Error in argument (expected type FLOAT): %s\n", value.c_str());
												 fprintf(stderr, "Options which expect infinite arguments should be last.\n");
												 exit(1);
											 }
											}
										}
										break;
				case TYPE::DOUBLE: {
										 if (xargs == 1) {
											 double* variable = static_cast<double*>(destination);
											 *variable = stod(value);
										 } else {
											 vector<double>* variable = static_cast<vector<double>*>(destination);
											 try {
											 variable->push_back(stod(value));
											 } catch ( ... ) {
												 fprintf(stderr, "Error in argument (expected type DOUBLE): %s\n", value.c_str());
												 fprintf(stderr, "Options which expect infinite arguments should be last.\n");
												 exit(1);
											 }
										 }
										 }
										 break;
				case TYPE::UINT: {
										 if (xargs == 1) {
											 unsigned int* variable = static_cast<unsigned int*>(destination);
											 *variable = stoul(value);
										 } else {
											 vector<unsigned int>* variable = static_cast<vector<unsigned int>*>(destination);
											 try {
											 variable->push_back(stoul(value));
											 } catch ( ... ) {
												 fprintf(stderr, "Error in argument (expected type UINT): %s\n", value.c_str());
												 fprintf(stderr, "Options which expect infinite arguments should be last.\n");
												 exit(1);
											 }
										 }
									  }
									  break;
				case TYPE::LONG: {
										 if (xargs == 1) {
											 long* variable = static_cast<long*>(destination);
											 *variable = stol(value);
										 } else {
											 vector<long>* variable = static_cast<vector<long>*>(destination);
											 try {
											 variable->push_back(stol(value));
											 } catch ( ... ) {
												 fprintf(stderr, "Error in argument (expected type LONG): %s\n", value.c_str());
												 fprintf(stderr, "Options which expect infinite arguments should be last.\n");
												 exit(1);
											 }
										 }
									  }
									  break;
				case TYPE::CHAR: {
										 if (xargs == 1) {
											 char* variable = static_cast<char*>(destination);
											 *variable = value[0];
										 } else {
											 vector<char>* variable = static_cast<vector<char>*>(destination);
											 try {
											 variable->push_back(value[0]);
											 } catch ( ... ) {
												 fprintf(stderr, "Error in argument (expected type CHAR): %s\n", value.c_str());
												 fprintf(stderr, "Options which expect infinite arguments should be last.\n");
												 exit(1);
											 }
										 }
									  }
									  break;
				case TYPE::STRING: {
											 if (xargs == 1) {
												 string* variable = static_cast<string*>(destination);
												 *variable = value;
											 } else {
												 vector<string>* variable = static_cast<vector<string>*>(destination);
												 try {
												 variable->push_back(value);
											 } catch ( ... ) {
												 fprintf(stderr, "Error in argument (expected type STRING): %s\n", value.c_str());
												 fprintf(stderr, "Options which expect infinite arguments should be last.\n");
												 exit(1);
											 }
											 }
										 }
										 break;
				default:
										 perror("(Warning! Unknown argument type.)\t");
										 break;
			}
		}
	}

	static void addp(TYPE _type, void* _destination) { // for the help flag
		priv::Param* addedparam = new priv::Param(_type, _destination, "--help", "Prints this help message.", 1, false);
		priv::params_map["--help"] = addedparam;
	}

	static void addp(TYPE _type, void* _destination, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, 1, true);
		priv::params_map[_longPhrase] = addedparam;
	}

	static void addp(TYPE _type, void* _destination, int _xargs, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, _xargs, true);
		priv::params_map[_longPhrase] = addedparam;
	}
	
	static void addp(TYPE _type, void* _destination, string _defaultValue, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, 1, false, _defaultValue);
		priv::params_map[_longPhrase] = addedparam;
	}

	static void addp(TYPE _type, void* _destination, int _xargs, string _defaultValue, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, _xargs, false, _defaultValue);
		priv::params_map[_longPhrase] = addedparam;
	}

	static void addp(TYPE _type, void* _destination, bool _required, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, 1, _required);
		priv::params_map[_longPhrase] = addedparam;
	}

	static void addp(TYPE _type, void* _destination, int _xargs, bool _required, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, _xargs, _required);
		priv::params_map[_longPhrase] = addedparam;
	}
	
	static void addp(TYPE _type, void* _destination, string _defaultValue, bool _required, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, 1, _required, _defaultValue);
		priv::params_map[_longPhrase] = addedparam;
	}

	static void addp(TYPE _type, void* _destination, int _xargs, string _defaultValue, bool _required, string _longPhrase, string _helpPhrase) {
		priv::Param* addedparam = new priv::Param(_type, _destination, _longPhrase, _helpPhrase, _xargs, _required, _defaultValue);
		priv::params_map[_longPhrase] = addedparam;
	}

	static void argparse(char** argv) {
		string args = ""; // all args as string
		vector<int> pos; // word positions [start, end, start, end, start, ...]
		for (int i=1; argv[i]!=nullptr; ++i) {
			args += string(argv[i])+" ";
		}
		int posspace=0, posquote=0, posequals=0;
		posequals=args.find(' ', 0);
		while(posequals != -1) {
			if (posequals > 0 && args[posequals-1] != '\\') {
				args[posequals] = ' ';
			}
			posequals = args.find('=', posequals+1);
		}
		int location = 0;
		posspace = args.find_first_not_of(' ', location); // get new word boundary start
		while(posspace != -1 && (unsigned long)posspace != args.length()-1) {
			if (args[posspace] == '"') { // if new word is string, then find end of string (not \", but ")
				pos.push_back(posspace+1); // skip the quotation mark
				posquote = posspace;
				location=posquote;
				posquote = args.find('"', location+1); // find end of string 
				location = posquote;
				pos.push_back(location);
			} else { // else new word is not a string, find end space boundary after recording beginning
				location = posspace;
				pos.push_back(location);
				posspace = args.find(' ', location+1);
				location = posspace;
				pos.push_back(location);
			}
			posspace = args.find_first_not_of(' ', location+1); // get new word boundary start
		}
		bool readingAnOption = true; // indicates if the read loop is trying to read a switch or argument
		priv::Param* parameter = nullptr; // parameter for which we're reading a value(s)
		int parameter_counter=1;
		for (auto i=begin(pos); i!=end(pos); ++i) {
			if (readingAnOption) {
				int beg=*i++;
				string key = args.substr(beg, *i-beg);
				if (priv::params_map.find(key) == priv::params_map.end()) {
					fprintf(stderr, "Unrecognized option '%s' in invocation.", key.c_str());
					exit(1);
				} else {
					parameter = priv::params_map[key];
					if (parameter->type != TYPE::BOOL) {
						readingAnOption = false;
						parameter_counter = parameter->xargs;
					} else {
						parameter->Set("true");
						if (key == "--help") {
							return;
						}
					}
				}
			} else { // reading an argument
				int beg=*i++;
				string key = args.substr(beg, *i-beg);
				parameter->Set(key);
				--parameter_counter;
				if (parameter_counter == 0) {
					parameter->set = true;
					readingAnOption = true;
				} else if (parameter_counter < 0) {
					parameter->set = true; // we will never stop reading arguments, as we assume infinite args in this case
				}
			}
		}
		for (auto& entry : priv::params_map) {
			if (entry.second->required && !entry.second->set) {
				fprintf(stderr, "Option '%s' required, and not found, or incomplete.\n", entry.second->longPhrase.c_str());
				exit(1);
			}
		}
	}

	static string argdetails() {
		string details="";
		for (auto& entry : priv::params_map) {
			details += "\t"+entry.second->longPhrase;
			details += "\n\t\t"+entry.second->helpPhrase;
			if (entry.second->xargs > 0 && entry.second->type != TYPE::BOOL) {
				details += "\n\t\t"+to_string(entry.second->xargs)+" argument";
				details += (entry.second->xargs != 1) ? "s" : "" ;
				details += " of type ";
				switch(entry.second->type) {
					case TYPE::BOOL: 
						details += "bool.";
						break;
					case TYPE::INT: 
						details += "int.";
						break;
					case TYPE::FLOAT: 
						details += "float.";
						break;
					case TYPE::DOUBLE: 
						details += "double.";
						break;
					case TYPE::UINT: 
						details += "unsigned int.";
						break;
					case TYPE::LONG: 
						details += "long.";
						break;
					case TYPE::CHAR: 
						details += "char.";
						break;
					case TYPE::STRING: 
						details += "string.";
						break;
				}
			}
			//printf("Reauired '%s'\n", (entry.second->required ? "true" : "false"));
			if (entry.second->required == false) {
				details += "\n\t\tdefault: '"+entry.second->defaultValue+"'";
			}
			details += "\n";
		}
		return details;
	}
}


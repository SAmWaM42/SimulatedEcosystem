// Mating rules shared by animals: validate a pairing, fix offspring genomes at
// conception, and charge both parents their configured energy costs.
#pragma once

class Animal;
class Simulation;

// Attempts conception; returns false if the parents are ineligible or the
// kind's population cap is already reached.
bool conceive(Animal& mother, Animal& father, Simulation& sim);

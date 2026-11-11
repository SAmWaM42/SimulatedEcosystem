# EcoSim genetics

## Genome layout

Each animal has one `Genetics::Genome`:

- `sight[2]` - two float alleles, each constrained to the default range
  **200-400**.
- `colour[2]` - two `ColourAllele` values ordered **Dark > Medium > Light**.
- `sexChromosomes[2]` - two `SexChromosome` values, `X` or `Y`.

Slot 0 is inherited from the mother and slot 1 from the father. Founders receive
two random sight alleles, two random colour alleles, and the chromosomes for
their assigned sex.

## Gametes and fertilisation

`makeGamete` chooses one of the parent's two sight alleles, one of the two colour
alleles, and one sex chromosome. It then applies mutation. `fertilise` copies the
maternal gamete into child slot 0 and the paternal gamete into slot 1.

A mother always contributes an X chromosome. A father contributes X or Y, so the
child is female for XX and male when a Y is present.

## Expression

- **Sight** is the mean of the two sight alleles and is cached in the animal.
- **Colour** uses complete dominance: any Dark allele gives Dark; otherwise any
  Medium gives Medium; two Light alleles give Light.
- **Sex** is male if either chromosome is Y, otherwise female.

## Mutation rules and defaults

- **Sight:** a gamete mutates with probability **0.05**. The selected allele is
  nudged by a Gaussian with mean 0 and standard deviation **10**, then clamped
  to 200-400.
- **Colour:** a gamete mutates with probability **0.01**. Its allele is shifted
  by one or two positions modulo the three colour alleles, so the result always
  differs from the unmutated allele.

The Punnett view compares the child's slot-0 sight allele with both maternal
alleles and its slot-1 sight allele with both paternal alleles; a value outside
those sets indicates mutation. The same test is performed for colour.

## How traits affect the simulation

Sight is both a maximum detection range and a probability of noticing a target.
Each animal scans every **0.3 s** (the default `senseInterval`). The chance to
notice a target at distance `d` within range `r` is `1 - d / r`.

- Herbivores use sight directly to find plants, mates, and predators. They flee
  when a predator is detected.
- Predators use their genetic sight multiplied by **2.0** as their hunting
  range. They stop hunting when the herbivore population is at or below the
  refuge of **20**.
- Higher sight costs energy every second:
  `(baseUpkeep + sightUpkeep * sight) * upkeepMultiplier * dt`. The defaults are
  `baseUpkeep = 0.5` and `sightUpkeep = 0.004`; predators apply a **0.2**
  upkeep multiplier.
- Mating also depends on energy and age: animals must be at least **15 s**
  old, have at least **60%** of maximum energy, not be pregnant, and not be on
  reproduction cooldown. Gestation lasts **12 s**.

## The neutral colour gene

The colour gene changes the rendered animal colour and the family/Punnett views,
but it has no effect on movement, feeding, defence, reproduction, or energy in
the current simulation. Its allele frequencies therefore change through
Mendelian sampling and mutation - genetic drift rather than selection. The
Statistics window presents that allele frequency as a neutral comparison with
the selected sight trait.

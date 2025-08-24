# mars_minwe

To begin, we should probably start with the most important part of any open-
source hobby project: Pronunciation.

<dl>
<dt>Mars Min W E</dt>
<dd>The original mars_nwe aimed to be a numbers-matching Netware server 
however, in 2025, my goal is much more minimalist: Whatever I need to 
proxy my old DOS machines to more modern network storage.</dd>
<dt>Mars My N W E</dt>
<dd>Initially I set out to slap a fresh coat of paint on mars_nwe but, 
unfortunately, the code base is showing its age.</dd>
</dl>

## Differences

### RIP 

* Will receive RIP packets from a network and update the kernel route table
* Will _not_ announce anything other than its internal IPX network
* Really doesn't understand hops and ticks

## Configuration

The repository contains a sample mars_minwe.ini which reflects various default 
settings. Note that, as it is 2025, you will need to define at least one 
`[network]` section as I do not want to spray IPX all over. My VM uses a 
second NIC attached to my Retro vlan and that's `device=`.

## Requirements

* The IPX kernel module
* A suitable Linux build environment (autotools, etc)


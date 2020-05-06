# Introduction

Fly8 is an old (90s) flight simulator originally written by Eyal
Lebedinsky.

Eyal published Fly8 with the permission: "It can be freely distributed in
the spirit of the GNU copyleft."

## Version 1.12 Beta (branch: 1.12)

The last official release of Fly8 was actually 1.11.  1.12 sat in Beta for
an extended period of time and was never rolled into a final release.

The [manual for 1.12](/docs/fly8-1.12.pdf) has been converted to PDF.

I acquired the most recent sources from Eyal back in 2008 after discovering
that the zip archives had fallen off of the internet.

I consider the 1.12 branch to be a record of living history - it's the
simulator as it was when Eyal stopped working on it, and in the form that
I remember it (I barely touched 1.11).

The only changes that I've made to the 1.12 branch has been tidying up
duplicated files (the fly8 build/config process would copy sources around
when you set it up to target a platform) and a few small adjustments to
play nice with git and modern tools

It can still be built, even if it doesn't run properly on most PCs due to
underflowing the FDM.  It can potentially still be used, as is, on slower
non-x86 systems and I may yet port it to the few I still run.


# Index

  1. What is this?

  2. Who will find this source code useful?

  3. Legal stuff

  4. What is the IEEE 1905 standard?
     1. IEEE 1905 overview
     2. IEEE 1905 components
     3. Limitations in the IEEE 1905 standard
        1. ALMEs are undefined
        2. Control CMDUs
        3. Unknown destination AL MAC address

  5. Build instructions
     1. Linux native compilation
     2. Linux cross compilation

  6. Usage instructions on Linux platforms
     1. Abstraction Layer Entity
     2. High Level Entity
     3. Libraries

  7. Hacking
     1. Source code organization
        1. Files structure
        2. Code overview
           1. Common component
           2. Factory component
           3. AL component
           4. HLE component
     2. Porting to new platforms
     3. Protocol extensions
        1. Files structure
        2. Register a group
        3. Code overview
           1. TLVs
           2. Callbacks
              1. CMDU extension
              2. Datamodel extension


  8. Testing
     1. Unit tests
     2. Static code analysis
     3. Runtime analysis
     4. Black box tests
        1. Push button configuration
           1. Scenario 1
           2. Scenario 2
           3. Scenario 3
        2. Access Point security settings cloning
           1. TC001
           2. TC002
           3. TC003
           4. TC004
           5. TC005

  9. TODO

  10. Appendix A: OpenWRT tool chain and flash image generation

  11. Appendix B: Installing "minGW"



# What is this?

This is a source code repository containing a 100% (or that's what I like to
think) **IEEE 1905 compliant implementation**. It covers both the original 1905
specification as well as the 1905.1a amendment.

> If you don't know what the IEEE 1905 standard is all about, I have written a
> small overview in a later section of this same document.

This implementation has been designed to make it as easy as possible to port it
to any platform: all you have to do is provide a dozen or so of functions on
which the code depends (such as functions to allocate memory or obtain
information regarding a network interface).

In addition to this "easy to port code", all the "platform-specific" functions
for *the Linux platform* are also provided. This means you can *now* checkout
the source code from this repository and make it work *right away* on almost
any Linux distro with minor or no modifications.

So, to make it clear, what you will find in this repository is:

  1. A 1905 implementation that is completely independent from other libraries
     and, in fact, only needs a handful of additional platform specific
     functions to run.

  2. That "handful" of additional functions already implemented for the
     particular case of a standard Linux platform.



# Who will find this source code useful?

As of today, I guess the target audience for this repository are developers
working on an embedded router who have been assigned the task to "implement"
the IEEE 1905 standard because the marketing department foresaw it was a
"killer feature".

Right now there doesn't seem to be any other open IEEE 1905 implementation,
however there are devices out there which say they are IEEE 1905 compatible.

If you are just about to start yet another closed implementation, I urge you to
consider the possibility of contributing to this repository instead. After all,
the best way to push a standard forward is to make it easy for everyone to
adopt it, right?

In addition, there is another reason to contribute: the IEEE 1905 is not exactly
the most refined standard in the world. It contains several holes open to
interpretation and some other items that are... well... "strange" at least.
Having a common open implementation is useful to "quickly" agree on some of
these issues so that the IEEE can later come up to speed (updating a standard
is a *slow* process and pushing from below, from the technical side, is always
a good idea).

In the future, maybe this repository  will be used to create Linux distro
packages than can be automatically installed with a single call to
"yum/aptitude/pacman/...".



# Legal stuff

This software is covered by the license contained in the "LICENSE" file. Also
see the further information contained in the "PROJECT" file (both files are in
the same folder as this "README.md").



# What is the IEEE 1905 standard?

Even if you *think* you know what IEEE 1905 is, you should read the next
sections.



## IEEE 1905 overview

Basically, the idea behind IEEE 1905 is to have a mechanism that lets home
devices discover each other and communicate. The purpose? Use optimal paths for
data transmission.

For example, let's say we have an ADSL home router with a power line and a WIFI
interface. At the other end of the house there is another power line device
connected to a computer like this:

```
    ADSL   --------       WIFI              ----
  =========|Router| ) ) ) ) ) ) ) ) ) ) ) ) |PC|
           --------                         ----
              *                              |
              *                              | Ethernet
              *                              |
              *        Power line         --------------
              * * * * * * * * * * * * * * | power line |
                                          | adapter    |
                                          --------------
```

If the computer tries to access the Internet... what interface should it use?
The Ethernet interface connected to the power line adapter *or* the WIFI
interface?

> Note that IEEE 1905 covers other types on interfaces too, such as Ethernet,
> coax, ...

IEEE 1905 helps devices decide: thanks to the information that travels inside
1905 messages each node in the network knows the network topology, the speed
of each interface, if there are congestion problems at some point... etc, and
then decides how to route traffic or even tells other devices how to route
theirs.

So... IEEE 1905 is "kind of" the IEEE alternative to Microsoft's "**Windows
Rally**" technology (which is what Windows uses to "draw" the network topology
when you go to "network properties" and lets it diagnose problems such as "your
router is not reachable" and things like that).

> On this note, Microsoft offers an open source implementation of the "Windows
> Rally" stack (I ported it to one custom platform a few years ago)... but it
> seems like it never really took off or else I suspect the IEEE 1905 standard
> would probably not have been created... or maybe it would, who knows...  At
> this point we just need to wait and see which one (if any) sticks.

But there is more: another (equally important) of IEEE 1905's main objectives
is to make it easy to add new devices to an already secured network. This
aspect of IEEE 1905 covers two scenarios:

 * One "standard" device, no matter its underlying technology (WIFI, power
   line, other...) can be added to an already encrypted network by pressing two
   buttons: one on the device to be added and another on *any* other device
   from the ones already authenticated (again, no matter its underlying
   technology).
   This is similar to today's WPS configuration mechanism (used to add new
   devices to a WIFI network), in fact IEEE 1905 **triggers** WPS behind your
   back for WIFI interfaces (and other mechanisms for other technologies).

 * One "WIFI access point" device (we are talking about WIFI here exclusively,
   at least in the current form of the standard) can "clone" the security
   settings of another (already configured and secured) one so that all "WIFI
   access points" on the network end up sharing the same SSID and password.

As you can see both of the previous points aim at making it easier for a home
user to configure their devices: they just press a button and new devices are
added and/or new access points are automatically configured.

The more devices on the network implement IEEE 1905, the more information will
be available to decide routes and the easier the configuration of new devices
will seem.

As of today (February 2016) the standard consists of two documents:

  1. The original "**IEEE Std 1905.1-1023**" document.

  2. The later "**IEEE P1905.1a/D1, July 2014**" update.

Unfortunately they are *not* freely available.

The source code contained in this repository implements all the functionality
detailed in both of these documents.



## IEEE 1905 components

What is an IEEE 1905 implementation made of? Well, there are two main
components:

  1. The **AL** (*Abstraction Layer*) is the component in charge of running the
     core protocol. It controls a node's interfaces and uses them to send
     periodic broadcast discovery messages to notify other ALs (running in
     other devices) about its presence.

     Once one of these discovery messages is received from a neighbor, the AL
     entity starts sending a series of unicast packets to query for all types of
     additional information: "*what interfaces do you have?*", "*of which type
     is each of them?*", "*which other neighbors are you aware of?*", etc...

     Obviously, the AL is also the one in charge of processing queries and
     sending the appropriate response when required by other ALs.

     All messages sent/received between ALs are called "**CMDUs**" and depending
     on their type they encapsulate one or more specific "**TLV**"
     ("type-length-value") blobs.
     For example, the CMDU that is used to search for WIFI access points
     registrars includes a TLV that specifies which WIFI band (2.4 GHz or 5
     GHz) the node performing the search supports (to filter unnecessary
     responses)

  2. The **HLE** (*High Layer Entity*), on the other hand, is the component that
     queries the AL (about all the information it has gathered using CMDUs) and
     then sends it **control messages** to modify its state (basically by
     creating/deleting new routing rules and/or changing the power state of one
     or more interfaces).

     These messages sent/received between one HLE and one AL are called
     "**ALMEs**" and, unfortunately, their bit structure or transport mechanism
     is **not** defined in the standard (only their content, in a more or less
     abstract way).

So... on one hand we have the ALs running on each node of the network,
periodically querying their neighbors and building a private (to each node)
database containing all types of information.
This communication takes place by using level-2 Ethernet frames precisely
defined in the IEEE 1905 standard.
```
  ----------              ----------       ----------
  | Node#1 |              | Node#2 |       | Node#3 |
  |--------|              |--------|       |--------|
  | AL#1   |              | AL#2   |       | AL#3   |
  ----------              ----------       ----------
      |                       |              |
      ----------------------------------------
         LEVEL-2      |
                  ------------------
                  | AL#1   |  HLE#1|
                  |-----------------
                  |     Node#4     |
                  ------------------
```

On the other hand we have one (or maybe more, but one should be enough) HLE(s)
(running on any node, which might or might not also contain an AL) querying the
ALs to build a "whole" network view and then, based on that, send control
commands to the ALs whose routes/interfaces need to be modified to improve the
network performance.
The bit structure and transport mechanism for these messages is, unfortunately,
left undefined in the standard.
```
  ----------              ----------       ----------
  | Node#1 |              | Node#2 |       | Node#3 |
  |--------|              |--------|       |--------|
  | AL#1   | <...         | AL#2   |       | AL#3   |
  ----------    : ?       ----------       ----------
      |         ............. |  ^           |   ^
      ----------------------:----:------------   :
         LEVEL-2      |     :    : ?             :
                  ------------------           ? :
                  | AL#1 <~|~ HLE#1|..............
                  |-----------------
                  |     Node#4     |
                  ------------------
```

So, remember:

  * ALs gather network information using TLVs embedded inside CMDUs that travel
    in level-2 packets.

  * HLEs query and control ALs using ALMEs whose bit structure and transport
    mechanism are not defined.

  * ALs are "dummy". HLEs is where an "intelligent" entity lives: it sends
    control command depending on the information obtained by previous queries.
    Note that this "intelligence" is also **not** defined in the standard.



## Limitations in the IEEE 1905 standard

When I started to implement the standard I (obviously) first read it **many**
times, finding out some limitations. It is difficult to understand the purpose
of some messages, a few features are introduced without enough context, use
cases are not present, and so on ...

I bring up this for three reasons:

  1. To "comfort" other developers that are now where I was a month ago (at some
     point I thought I was stupid and reading words like these would definitely
     have helped)

  2. To bring some attention to this, hopping that IEEE members will work on
     this in the future.

  3. To introduce the next list of specific problems that affect the current
     implementation (found on this repository) in some way

From all the issues discovered, some of them are worth discussing as they somehow
affected my implementation. Let's review them:



### ALMEs are undefined

This is quite a limitation, because right now, with the information available in
the standard, ***an HLE from one implementation won't be able to communicate
with an AL from a different implementation***.

Two things are missing:

  * The bit structure of ALME messages ("*how big is each field?*", "*in which
    order do they appear?*", ...).

  * The transport mechanism ("*where is the ALME payload embedded?*", "*in
    level-2 or level-3 frames?*", "*if level-3, what type?*", "*how is
    authentication handled?*", ...)

> Regarding this "limitation" I have heard that the "unspoken consensus" among
> members of the IEEE group in charge of this standard is something like "yeah,
> well... use uPnP or something like that", but yet the actual bit structure
> remains undefined.
> I have also seen other closed implementations that encapsulate these ALME
> messages **inside** (I kid you not!) the level-2 CMDUs used by ALs, using a
> TLV reserved for "vendor extensions".

One of the strengths of having an open implementations (such as this one!) is
that we can agree on (and implement!) a bit structure and transport channel for
these ALME messages making interoperability possible and paving the way for
future standard amendments.

More specifically, in this implementation you will find an arbitrary bit
structure for ALMEs (which I have decided by myself... but I promise it makes
sense) and, in addition, in the Linux port, a simple TCP (level-3) connection
is used to transmit the payload.

In the future I would like to somehow authenticate that TCP channel so that
only commands from an HLE running in the "network administrator owned node" are
accepted.



### Control CMDUs

The 1905 standard has both redundant messages and security issues.

Let me explain this in more detail.

The original 1905 standard does not have "control" CMDUs: all the CMDUs that
existed did not affect a node's state, instead they were only used to "gather"
information (which is a nice thing: ALs only gather information).

"Control" commands were delivered to nodes by using ALME messages instead of
CMDUs, i.e. "control" commands can only be delivered from an HLE to an AL.

> As we have previously said the transport mechanism for ALMEs is not defined
> but let's just assume they go inside an SSH tunnel or something like that
> (instead of inside an unencrypted unauthenticated L2 frame, as it is the case
> for CMDUs).

So far, so good.

However the "1a" update to the standard came next and it introduced a new TLV
that contains a control command to switch on an off interfaces on a remote
host.

This may be **problematic**: in effect this gives *any* node power to switch
on/off interfaces of any other node just by being on the same encrypted
network.

> Think of this in the following way: when someone comes to your house and you
> give them your WIFI password, they gain access to your network (e.g. they can
> access the Internet) but they cannot switch on/off interfaces on the router
> (for that they would also need to know the router password).
> However, with a "control" CMDU, any device on the network automatically gains
> "administrator" privileges!

But the introduction of this new "control" CMDU is **even more
incomprehensible** when you consider that the original 1905 *already* contained
and ALME message to change an interface power mode. The HLE just needed to
make use of this to directly send the desired command to a remote AL.

Because of all of this I really feel these control CMDUs (and all others that
might be introduced in the future) should not be used.

That's why I have created a special compilation flag named
"**DO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS**".

If you define that flag ("-DDO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS"), then these
CMDUs will be ignored (as of today, that means only the "interface power change
request" CMDU will be ignored). This has two effects:

  1. You will be more "secure"

  2. You won't be 100% compliant with the standard.

So it us up to you to decide what to do. Personally I would always set the flag.



### Unknown destination AL MAC address

This is a minor problem, but I had to make a choice regarding how I dealt with
this situation.

It turns out that, according to "Table 6.2" of the standard, all unicast CMDUs
must be sent to the AL MAC address of the receiving 1905 entity.  *However*,
this AL MAC is not always known. For example:

  1. A new node is started. It sends a "topology discovery message".

  2. Another node receives this message and decides to query the new node by
     sending a "topology query message".

  3. The new node receives this query but **because the topology query CMDU does
     not include an AL MAC TLV**, there is no way for the new node to know to
     which AL MAC address the response should be sent to.

This happens because the new node has never received a "topology discovery"
from any other node yet (remember that it has just entered the network).

There are two options here:

  1. Do not send the response *until* the AL MAC address of the destination is
     known (i.e.. wait for the destination node to send a "topology discovery"
     message).

  2. Send the response to the "src addr" field of the Ethernet frame containing
     the "topology query message".

In this implementation I have decided to issue a "WARNING" on STDOUT and then
send the response using the Ethernet source address (i.e. option "2" from above)

> Proposed standard improvement: If the standard said that "unicast CMDUs can be
> sent to either the AL MAC address or the interface MAC address" of the
> receiving  1905 entity, this problem would disappear.



# Build instructions

Before detailing platform specific compilation procedures, let me list some
platform independent compilation flags.

These are set in the make "*Makefile*" file. You can either comment or uncomment
them depending on the desired runtime behavior:

  * **DO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS**: This flag is explained in the
    "Limitations in the IEEE 1905 standard" section of this same "README.md". In
    short, when this flag is defined, if a "control" CMDU is received, it will
    be ignored. You become more "secure" but less "standard compliant". I
    personally like to **define** this flag.

  * **SEND_EMPTY_TLVS**: Some CMDU responses can include "zero or more" (or
    "zero or one") TLVs of a specific type. For example, the "topology
    response" CMDU can include "zero or more" "device bridging capability" TLVs.
    It would be "sane" to think that if a node does not have any "bridge"
    entity defined, it should send "zero" TLVs of this type. However, it is
    also possible to send "one" TLV that contains "zero bridges" entries.
    By default this flag is **undefined**, meaning the TLV won't be sent at all.
    It you define it, the "empty" TLV will be sent.
    I would say that the standard "spirit" is to **not** send the TLV, but
    thanks to the way it is written, it is open to interpretation.
    I personally like to let this flag **undefined**.

  * **FIX_BROKEN_TLVS**: When this flag is set, if a TLV contains a length of
    zero when it shouldn't (for example in an "interface power change status",
    whose minimum allowable length is "1"), instead of discarding it, it is
    treated in a "reasonable way" (for example, as if the needed fields to
    indicate a zero number of entries were present). This way you obtain
    greater compatibility with other implementations, but you become less
    "standard compliant". I personally like to let this flag **undefined**.

  * **SPEED_UP_DISCOVERY**: When this flag is set, when a node receives a
    "Topology Discovery" from a "new" node (i.e. a "node" it hasn't seen before),
    if addition to all the regular queries, we also send a "Topology Discovery"
    message back. This way the "new" node does not have to wait up to 1 minute
    to "discover" (and start querying) us.
    The standard "seems to imply" (although it is not 100% clear) that this
    is not the way to operate: instead, "new" nodes are expected to wait up to
    1 minute to discover the rest of already existing nodes (and this is done
    to avoid "mini-storms" of packets when a new node enters the network).
    Even if this parameter might cause mini-storms, I personally like to
    **define** this flag.

  * **REGISTER_EXTENSION_xxx**: When this flag is set, the protocol extensions
    belonging to the group 'xxx' are registered. 'xxx' identifies a particular
    group (ex. REGISTER_EXTENSION_BBF). Each group defines a set of non-
    standard TLVs which fill a gap in the standard. This is the current group's
    list:
    - REGISTER_EXTENSION_BBF: add non-1905 link metrics info


Remember that for maximum standard compliance you must:

  * **Not define** "DO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS"
  * **Not Define** "SEND_EMPTY_TLVS"
  * **Not Define** "FIX_BROKEN_TLVS"
  * **Not Define** "SPEED_UP_DISCOVERY"

...but I personally would:

  * **Define**     "DO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS"
  * **Not Define** "SEND_EMPTY_TLVS"
  * **Not Define** "FIX_BROKEN_TLVS"
  * **Define**     "SPEED_UP_DISCOVERY"



## Linux native compilation

> Note: by "native compilation" I mean that you are building the binaries in
> the same platform where they are meant to be run

The root "*Makefile*" already has the "*PLATFORM*" variable set to "*linux*"
and the *FLAVOUR* variable set to "*x86_generic*", so you don't really need to
do anything special. Simply clone this repository, and execute the following
command:
```
  $ make
```

Note that the Linux port depends on the following libraries:

  * "**libpthread**" and "**librt**" to create threads this are typically
    already installed in all Linux systems.

  * "**libcrypto**" for WPS stuff. As in the previous case you probably need to
    install it first (ex: "sudo aptitude install libssl-dev" in Debian based
    distros)

... thus you need to have the corresponding "*\*.so*" and "*\*.h*" files in
place before executing "make".

The whole build process takes a pair of seconds at most. Once it finishes go to
the "*output*" folder where you will find two executable files:

  * **al_entity**: This is the AL "daemon"
  * **hle_entity**: This is the limited HLE command line tool



## Linux cross compilation

> A cross compilation situation would happen, for example, when you are trying
> to build the binaries in a Linux PC (x86) host *but* the binaries themselves
> are meant to be run in a Linux ARM embedded platform.

In some "flavours" (e.g. when "*PLATFORM*" is set to "*linux" and
"*FLAVOUR*" is set to "*arm_wrt1900ac*") a cross compiler is used instead of the
default system compiler.

Simply set the "*PLATFORM*" and "*FLAVOUR*" variables before calling ```make```
and proceed as in the previous section. Example:
```
  $ PLATFORM=linux FLAVOUR=arm_wrt1900ac make
```

In the "Porting to new platforms" section you will find details regarding how
to obtain the cross compiler needed for each of the currently supported flavours
and what requirements are needed in the target platform (e.g. which shared
libraries, etc...)

The output binaries are located in the same folder as before ("*output*").



# Usage instructions on Linux platforms



## Abstraction Layer Entity

The AL entity is executed like this:
```
  $ ./al_entity -m <al_mac_address> -i <interfaces_list>
```
...where "*\<al_mac_address\>*" is the MAC address of the AL entity (you must
provide one, as there is no place in the system from which such address can be
consulted) and "*\<interfaces_list\>*" is a comma separated list of interfaces
names you want the AL entity to manage (ex: "*eth0,eth1,wlan0*").

The AL entity daemon accepts several other parameters. Execute it with
"*--help*" to see a list of all possible arguments (in particular pay attention
to the "*-v*" argument that turns on the "verbose mode").

Once the daemon is running it will remain there until you kill it.

> Note: Even if I am calling it "daemon", it is a standard process that sends
> output to STDOUT. In the future I might add the option to turn it into an
> actual daemon by using an extra parameter.
> This means that, for now, in order to kill it, you just need to press CTRL-C
> in the same terminal where the AL entity was started.

The Linux platform includes two "external triggers" that you need to be aware
of:

  1. The **"virtual push button" trigger** is a file named
     **"/tmp/virtual_push_button"**. Whenever you "touch" this file (ex:
     ```touch /tmp/virtual_push_button```), the AL entity will act as if the
     physical button that starts the "push button configuration method" had been
     pressed.
     The code for each Linux flavour also includes the "glue" needed to start
     the process when a real button (associated to a GPIO pin) is pressed, but
     in some cases it comes really handy to also have this "virtual" button.

  2. The **"topology change notification" trigger** is a file named
     **"/tmp/topology_change"**. Whenever you "touch" this file (ex:
     ```touch /tmp/topology_change```), the AL entity will act as if a real
     topology change had been detected.
     Right now this is the only way to "detect" topology changes (i.e. you must
     "manually" force them). In the future the code will listen to both changes
     in this "virtual" trigger **and** changes in the Linux kernel itself, but
     for now remember you are in charge of this (using, for example, an external
     script that monitors network interfaces and "touches" the virtual trigger
     when a change is detected).
     Detecting topology changes is not mandatory but it "speeds up" the whole
     1905 protocol (thus, it is a nice feature to have).

> HINT: Look inside the 'scripts' folder for auxiliary scripts for specific
> flavours that already take care of managing these "external trigger" files



## High Level Entity

The (limited) HLE entity is executed like this:
```
  $ hle_entity -a <ip address>:<tcp port> -m <ALME request type> [ALME arguments]
```
...where "*\<ip address>:\<tcp port>*" specify the location of the remote AL to
which we are going to send the ALME query/command.
> Remember that ALMEs bit stream and transport mechanism are undefined in the
> standard, thus you will only be able to communicate with AL entities that have
> decided to use the same bit stream and transport (e.g. a simple TCP socket) as
> this implementation.

The HLE entity tool accepts several other parameters. Execute it with
"*--help*" to see a list of all possible arguments, including the list of
available ALME messages and their arguments.

This HLE tool, as we have explained elsewhere, is limited in the sense that it
simply sends a query/command, waits for a response, prints it to STDOUT, and
exits.

For now there is no "daemon" mode that automatically queries ALs by itself,
takes decisions to improve network performance and then issues commands to those
ALs whose state requires to be modified.

> This might be implemented in the future, but remember that none of this is
> covered by the standard, thus I will probably just create a "strategy
> container" where each network operator can decided what the actual
> intelligence driving the HLE is.



## Libraries

In addition to the "al_entity" and "hle_entity" binaries, you will also find
some static libraries that you can use to forge/parse 1905 packets.

The main usage for this is to implement your own "hle_entity", capable of taking
"smart" decisions (instead of simply act as a "querier", like now).

> In any case, I would really like to make the default "hle_entity"
> implementation smarter, so if you are thinking about using these libraries for
> this purpose, please consider contributing to expanding the current
> implementation instead!

You can also use them to create custom utilities for network visualization and
such... (ex: query an AL, retrieve information, query more ALs, ..., and draw
network topology on screen).



## Misc considerations

In the Linux port, when two devices are bridged together (using "brctl"), before
running the "al_entity", you need to tell the Linux kernel to *not* forward
packets addressed to the 1905 multicast address. This is done by executing
this command as root:
```
  # ebtables -A FORWARD  -d 01:80:c2:00:00:13 -j DROP
```

The reason is that when you create a bridge of two interfaces, the kernel
automatically forwards multicast packets received on port A to port B, *however*
when the AL entity runs, it should be its job to decide which of these multicast
packets are forwarded (or not) on each interface (and not the Linux kernel's).

The only way to tell the kernel not to forward these packets is using the
above command.

> NOTE: There might be a way to do it "programmatically" when the "al_entity"
> starts. This will probably be implemented once the "bridge" processing part
> of the 1905 is reviewed.



# Hacking

If you want to contribute to this repository (and I would *love* it if you
do!), please consider reading the next sections first: the better you
understand the code structure, the more sense your modifications will make
inside the overall picture.



## Source code organization



### Files structure

At the root folder we have two folders (**src** and **output**) and two files
(**README.md** and **Makefile**)

```
    .
    |-- Makefile
    |-- README.md
    |-- LICENSE
    |-- PROJECT
    |-- version.txt
    |
    |-- output
    |   `-- tmp
    |
    |-- src
    `-- scripts
```

This is how they work:

  * **src** contains all the source code (who would have guessed!)

  * **output** is where intermediate objects, libraries and executables are
    placed by the build process.

    Initially the folder is empty... well, it actually contains another folder
    named "**tmp**" which itself contains an empty (and hidden) file called
    **.place_holder**, which is needed because "git" (the version control tool
    I use) does not accept empty folders.

    When the build process is triggered, intermediate objects are placed in
    "*output/tmp*" while libraries and executables are directly placed in
    "*output*".

    In other words, *nothing* from the *src* folder changes when you build
    (which is always nice)

  * **Makefile** is where the build instructions are defined. Refer to the
    "build instructions" section for more information.

  * **README.md** is *this* file you are reading

  * **LICENSE** contains the license information (see the "Legal stuff" section
    in this same document)

  * **PROJECT** contains the project information (see the "Legal stuff" section
    in this same document)

  * **version.txt** contains a "version tag". These version tags are made of
    the letter "i" (from IEEE1905) and a monotonically increasing number (ex:
    "i245"). They reflect the most recently applied git tag with same name, this
    way, if someone receives the source code in a zip package (without git
    information) we can always know from which git commit it was extracted.

  * **scripts** contains auxiliary scripts for specific platforms/flavours that
    might come handly to start/manage the 1905 binaries in those targets.
    These are only needed at runtime and you should consider them as basic
    templates that require modifications for your particular use cases.
    This folder contains another 'README.md' file with more details.


Let's next then dive into the "*src*" folder.

Recall that the IEEE 1905 standard defines two distinct objects: the **AL** and
the **HLE**. Well, each of these components has a dedicated folder: **src/al**
and **src/hle**. In addition we also find here two additional folders:
**src/common** and **src/factory**:

```
    .
    `-- src
        |-- al
        |
        |-- hle
        |
        |-- common
        |
        `-- factory
```

Each of these folders "produces" a special type of object when the build process
is triggered:

  * The "**al**" folder contains the source code for the "core" AL component.
    When compiling files inside this folder, intermediate object files are
    placed inside the "*output/tmp*" folder and they are all named
    "*\_al\_\<something\>.o*", while the final executable is placed inside
    "*output*" and is called "*al_entity*"

  * The "**hle**" works exactly in the same way: its intermediate files (called
    "*\_hle\_\<something\>.o*") are placed inside "*output/tmp*" while the final
    executable ("hle_entity") goes to "output"

  * "**common**" and "**factory**" are different from the previous two. They
    also produce intermediate files ("*\_libcommon\_\<something\>.o*" and
    "*\_libfactory\_\<something\>.o*") in "*output/tmp*" and their final
    executable is also placed in "output", **however** their final executable is
    not actually an executable, but a **library** which is used by others to
    produce their final binaries.

    "*common*" includes very "generic" functions, such as one to print to
    STDOUT.

    "*factory*" includes more 1905 specific functions, but only if they are
    needed by both the AL and HLE (otherwise they would be directly defined
    inside the "*al*" or "*hle*" folders).
    This mainly includes functions to parse and forge protocol packets (CMDUs,
    ALMEs, ...)

This is how the "*output*" folder looks like after the build process finishes:
```
    `-- output
        |-- al_entity
        |-- hle_entity
        |-- libcommon.a
        |-- libfactory.a
        `-- tmp
            |-- _al_*.o
            |-- _hle_*.o
            |-- _libcommon_*.o
            `-- _libfactory_*.o
```

Back to the "*src*" folder, each of the four "components" ("*al*", "*hle*",
"*common*" and "*factory*") contains a "*Makefile*" and zero or more of the
following sub-subfolders:

  * "**src_independent**": contains platform independent code (typically "core"
    1905 functions)

  * "**src_\<platform\>**": contains platform specific code (right now the only
    ported platform is "*linux*", thus the folder will be called "*src_linux*",
    but in the future there might be several of these subfolders).
    Platform specific code includes things such as functions to send or receive
    RAW packets or to obtain Tx/Rx stats from an interface.

  * "**internal_interfaces**": contains header files **internal** to the
    component. This means that, for example, functions declared in
    "*al/internal_interfaces*" can only be used from other files inside the
    "*al*" tree.
    Note that there are also header files inside the "*src_independent*" and
    "*src_\<platform\>*" folders. The difference is that these header files can
    only be used from files inside the same absolute folder, while header files
    from the "*internal_interfaces*" folder can be defined in
    "*src_independent*" and used from "*src_\<platform\>*" or the other way
    around... meaning that "*internal_interfaces*" is the "gateway" between
    platform dependent and platform independent code.

  * "**interfaces**": contains header files with the declaration of functions
    that are meant to be used from other components.

This is the actual folders layout:
```
    `-- src
        |-- al
        |   |-- internal_interfaces
        |   |-- src_independent
        |   `-- src_linux
        |
        |-- common
        |   |-- interfaces
        |   |-- src_independent
        |   `-- src_linux
        |
        |-- factory
        |   |-- interfaces
        |   |-- src_independent
        |   `-- unit_tests
        |
        `-- hle
            `-- src_linux
```

Note that only library components (i.e. "*common*" and "*factory*") have the
"*interfaces*" folder. This makes sense, as libraries are the only components
whose functions are meant to be used by others.

In particular, "*common*" interfaces are used by everyone (even by the
"*factory*" library itself!), while "*factory*" interfaces are only used by
"*al*" and "*hle*"

```
   Components dependencies:
   =======================

   hle ---------------------------
                |                |
                V                V
               factory ------> common
                ^                ^
                |                |
   al ---------------------------
```

At the file level, this is how dependencies interconnect the different
components. It is important that you spend a few minutes trying to fully
understand this, so that if you ever need to add a new file/interface you do so
in a way that the dependency structure is respected.
```
                       -----------------------------------
    `-- src            |                                 |
        |-- al         V                                 |
        |   |-- internal_interfaces                      |
        |   |          |                               __|_____
        |   |          V       _                      |        |
        |   |-- src_independent | <---. Use code from each other
        |   |                   |     | and also from "factory" ------
        |   `-- src_linux      _| <---' and "common" -------         |
        |                                                  |         |
        |                                                  |         |
        |-- common                                         |         |
        |   |-- interfaces     <--------------------+-+-----         |
        |   |                  _                    ^ ^              |
        |   |-- src_independent | "self             | |              |
        |   `-- src_linux      _| sustained"        | |              |
        |                                           | |              |
        |-- factory                                 | |              |
        |   |-- interfaces     <------------------..| |..--------+----
        |   |                  _                    | |          |
        |   `-- src_independent_| Uses code from "common"        |
        |                                             |          |
        `-- hle                _                      |          |
            `-- src_linux      _| Uses code from "common" and "factory"
```



### Code overview



#### Common component

The "*common*" library produces, as we have already explained, a "*libcommon.a*"
file whose functions can be used by others through the header files found in
"*src/common/interfaces*".

The functions you will find here fall in one of the following categories:

  * **Generic packet manipulation**: to insert/extract fields of 1, 2, 4 or n
    bytes to/from a bit stream in an endianness independent way.

  * **Generic platform functions**: wrappers to fundamental platform functions
    such as `memcpy()`, `malloc()`, `free()`, `printf()`, etc...

  * **Generic utilities**: these are arbitrary functions that are used from
    many places and don't have a better place to exist.

It would be interesting to keep the functionality provided by this library to
a minimum. In particular, the **less** "generic platform functions" we need, the
better (it will make it easier to port our code to different platforms in the
future).

> Let's see an example of this: imagine you are implementing some new
> functionality from a future update to the standard and you find that it would
> come handly to use `strncpy()` from platform independent code.
> One possible option would be to create a new wrapper function in the
> "*common*" library (let's call it `PLATFORM_STRNCPY()`) that simply calls
> `strncpy()` on a Linux platform, but this would add a new platform stub
> increasing the amount of effort/time it would take to port to a new platform.
> Another option (and, I think the best one) is to make use of the already
> existing functions `strlen()` and `memcpy()` to achieve
> the same functionality as `strncpy()`



#### Factory component

The "*factory*" library produces, as we have already explained, a
"*libfactory.a*" file whose functions can be used by others through the header
files found in "*src/factory/interfaces*".

These functions are meant to **parse** bit streams into memory structures and
the other way around (i.e. to **forge** bit streams from memory structures).

In particular, you will find the following header files (among others) in the
"*src/factory/interfaces*" folder:

  * **1905_alme.h** : Functions to parse/forge ALME payloads
  * **1905_cmdus.h**: Functions to parse/forge CMDUs
  * **1905_tlvs.h** : Functions to parse/forge TLVs

In particular there is **one** function to parse and **one** function to forge
on each of these files. That way they work is by accepting "void" pointers to
all types of structures/bit streams and the return the correct
bit stream/structure by first inspecting the element to find its type.

> Functions are **all** heavily documented. In fact (and this applies to all
> functions from all components), before you touch a function implementation,
> read the documentation contained on its header file to make sure you are
> respecting its interface (e.g. "what it is supposed to do").
> In particular, pay attention to memory management: some functions are expected
> to free passed pointers while others aren't... and this is always explained in
> the header documentation.

Each of these files contains, in addition to the functions prototype, the
structures definitions for each type of supported "payload/blob".
For example, file "*1905_tlvs.h*" contains the following structure
declaration:
```
     struct deviceIdentificationTypeTLV
     {
         struct tlv tlv;
         uint8_t  friendly_name[64];
         uint8_t  manufacturer_name[64];
         uint8_t  manufacturer_model[64];
     };
```
...which will be returned by the corresponding "*parse*" function when applied
on a bit stream containing a TLV of type "Device Identification" such as this
one:
```
  0x15, 0x00, 0xc0, 0x54, 0x76, 0x20, 0x60, 0x6e, 0x20, ...
```

> Note that the bit stream format for ALME messages is **not** defined in the
> standard, and thus functions in "*1905_alme.h*" will "parse from"/"forge to"
> a non-standard bit stream (one that was decided for this implementation).
> Refer to the "ALMEs are undefined" section of this same README.md for more
> information.

Each of these files also provides a few other functions to free, traverse
and/or compare structures. Remember that, in the future, if a new type of TLV,
CMDU and/or ALME is added, you need to make modifications in all of these
functions.

Finally, there are other additional header files I have not mentioned:

  * **lldp_payload.h** and **lldp_tlvs.h** functions are used to parse/forge
    (and free, traverse, compare...) "**LLDP**" structures/bit streams. LLDP is
    a packet that has nothing to do with IEEE 1905 except for the fact that an
    AL entity needs to parse incoming LLDPs to infer network topology
    characteristics.  In addition, if the node does not contain some other
    process (which is typically the case) to produce LLDPs periodically, the AL
    entity has to do it by itself.

  * **media_specific_blobs.h** functions are used to parse "media specific"
    structures/bit streams. These are not covered by the standard either, and
    right now we support a very small list of them. This is the file were new
    types should be added in the future in case more are needed.

  * **1905_l2.h**: here you will simply find L2 eth types and multicast
    addresses definitions.

So, remember, the "*libfactory.a*" is used to forge/parse all types of protocol
specific structures/bit streams, and that's why it is used by both the AL and the
HLE component.



#### AL component

The AL entity consists of a single function (`start1905AL`) that never returns
and that platform specific code has to call once the runtime environment is
ready.

This function simply waits for a message to be available in a system queue,
then processes the message and blocks again waiting for a new one. This
goes on forever.

Messages to this queue are posted by platform specific code when certain things
happen (a new level-2 packet arrives, a physical button is pressed, a timer
times out, etc...).

```
   platform independent code          platform dependent code
   =========================          =======================
   start1905AL()             <------  main()
   |
   |--> register queue events ------> start thread #1 (capture 1905 packets on interface #1)
   |                                  start thread #2 (capture 1905 packets on interface #2)
   |                                  start thread #3 (timer)
   |                                  start thread #4 (button monitor)
   |                                  start thread #5 (ALME message transport channel)
   |                                  ...
   |                                  start thread #N (...)
   |                                \_______________________/
   |                                          |
   |                                          |
   `-> while (1)                              | Post messages to
       {                                      | queue
          m = read_queue()  <------------------

          process_message(m)
          |
          |---------------------------> query interface
          |
          |---------------------------> send L2 packet
          |
          |---------------------------> send ALME reply
          |
          `---------------------------> change interface/route/bridge status
       }
```

Each platform port might choose to deliver messages to the system queue in the
way they prefer. For example, the picture from above roughly corresponds to the
Linux implementation that you will find in this repository, where I have chosen
to create different threads for different tasks that all post to the same POSIX
queue from where the main task reads.
Of course there are other ways to do it... the important thing to take away from
all this is that platform independent code interacts with platform dependent
code by means of a queue.



#### HLE component

The HLE component included is a very "simple" one meant to serve as a template
for more complex entities. In particular, this HLE is a command line tool that
sends an ALME message to an AL entity, waits for a response, and then shows its
contents on the screen. That's it.

A "real" HLE would need to query one or more nodes from the network, gathering
as much information as possible and then, based on some strategy, take some
decisions that translate into ALME commands.

Because this strategy is left open to the network operator, we don't do more
than what I have already described.

> HOWEVER, it would be nice to have optional "strategy modules" implemented!
> This is a nice feature to implement and contribute back ;)



## Porting to new platforms

If you want to make this code run on your platform, you should first understand
what I mean by "platform" and "flavour"... because in most cases what you will
actually end up doing is creating a new "flavour" for an already existing
"platform":

  * A "platform" is defined mainly by the underlying OS, its API and some common
    tools. For example, the "linux" platform (already supported in the current
    implementation) porting layer relies on several Linux specific things:

      - A POSIX API for queues and threads.
      - A Linux-like file system (with "/sys/class/net/", "/proc", etc...)
      - Some standard routing tools (ex: "btrctl")
      - ...

    Most of the "PLATFORM_*()" functions, when using the "linux" port, make use
    of these APIs/services to accomplish their goal (for example, the
    "PLATFORM_CREATE_QUEUE()" functions uses POSIX's "mq_open()")

  * A "flavour" is used two distinguish minor differences inside one same
    "platform".
    Right now we support three "flavours" for the "linux platform":

      - "**x86_generic**": This is the default flavour. Used to run on a
        standard Linux distro. No special assumptions are made. This is meant to
        be as generic as possible (and a good start point for creating new
        "flavours")

      - "**arm_wrt1900ac**": A configuration that produces binaries that can run
        on a WRT1900AC (or WRT1900ACS) Linksys router flashed with an OpenWRT
        linux distribution (i.e. replacing the original vendor firmware).
        When this flavour is selected, a cross-compiler is used. In addition,
        some tweaks in the source code are activated that deal with the way
        OpenWRT configures things (e.g. information regarding WIFI
        interfaces is retrieved using OpenWRTs "uci" tool)
        > IMPORTANT: Two things are needed in order to be able to use this
        > flavour: the cross compiler and the OpenWRT flash image. Both are
        > generated using OpenWRT build scripts. These scripts accept an input
        > configuration and generate an output tool chain (cross compiler) and a
        > flash image. Note that the "default" tool chain and flash image for
        > the WRT1900AC(S) that can be downloaded from the OpenWRT webpage do
        > not work, as their input configuration does not include, among other
        > things, support for pthreads in the kernel (which is needed by our
        > IEEE1905 implementation).
        > In other words, you must build the tool chain and flash image yourself
        > using the OpenWRT build scripts and a special input configuration.
        > The detailed instructions on how to do this are included in "Appendix
        > A" at the end of this same "README.md" file

      - "**x86_windows_mingw**": A configuration that produces binaries that can
        run on Windows!
        I know what you are thinking: "this should be a whole new platform, and
        not a flavour of the Linux platform"... and you are right. However,
        when using the "minGW" cross compiler (a cross compiler capable of
        producing Windows binaries on Linux), the modifications required in the
        source code of the Linux platform are so few that it becomes more
        practical to consider this as a flavour (just like in the case of the
        Linksys WRT1900AC(S) flavour). Otherwise (i.e. if we defined a new
        platform) we would have to replicate all the platform specific folders
        and files.  Note however that the "minGW" cross compiler comes with an
        important limitation: it offers a **very minimal** POSIX compatibility
        layer. This means you cannot, for example, use threads among many other
        things.  Because of this, when this flavour is selected, **only** the
        "**hle_entity**" binary (which is very simple) is built. For the
        "al_entity" to even compile a much greater porting effort would be
        needed (this time creating a "real" new platform instead of just a
        flavour).
        The good news is that on the typical scenario, Windows computers are
        "end points" that don't really need to run the "al_entity". They are
        more than happy with the "hle_entity" which lets them retrieve all kinds
        of information regarding the surrounding network equipment (specially
        when the "hle_entity" is being "managed" from a GUI tool capable of
        displaying network connections and data)
        > IMPORTANT: In order to use this flavour you need the "minGW" cross
        > compiler installed. "Appendix B" (at the end of this same "README.md"
        > file) explains how to do it.



### Adding a new "flavour" to a "platform"

According to what we have just said, in the **most typical scenario** you will
want to port this IEEE1905 code to your Linux-powered device. If that's the
case, you need to create and add support for a new "flavour":

  1. In file "Makefile", under the "linux" platform section, add your new
     flavour. Name it something descriptive.

  2. In the new "flavour" section (still in that same "Makefile") specify the
     new compiler path and special compilation/linking flags your device needs.
     In addition, create a new compilation flag to "CCFLAGS" with the name of
     your flavour (ex: "CCFLAGS += -D_FLAVOUR_MY_FLAVOUR_NAME_")

  3. Now modify files inside any of the subfolders named "src_linux" (and only
     those files!) and use the new compilation flag
     ("_FLAVOUR_MY_FLAVOUR_NAME_" in the example) to perform things in a
     device-specific way. Example:
     ```
       #ifdef _FLAVOUR_MY_FLAVOUR_NAME_
         <your new code that performs things in a device-specific way>
       #else
          <already existing code>
       #end
     ```

Most Linux distros are similar, and these "flavour" flags are meant to address
very specific behaviors such as:

  - The way you can access interface info (example: in some distros you can
    directly read files under "/sys/class/net", in others, however, maybe you
    have to call "ifconfig" or "ip")
  - The way your device exposes the physical buttons (do I have to take control
    of the GPIO using the "/sys/class/gpio" kernel API? Is there a script
    already taking care of this? How do I interact with it?)
  - The way WIFI configuration settings is applied (how do I change the WIFI
    settings? Writing to a file? Using a netlink socket?)
  - Others?



### Adding a new whole "platform"

If you want this source code to compile for a device that runs something
completely different from Linux (such as an embedded platform running a custom
RTOS or something like that), *then* (and only in this case) you will have to
create a new "platform".

This is not a difficult task, but it requires much more effort than simply
creating a new "flavour" as explained in the previous section.

These are the basic steps you should follow:

  1. Whenever you see a folder called "src_linux", create a new one called
     "src_<your_platform_name>".

  2. In these folders, create files that contain the implementation of all the
     needed "PLATFORM_*()" functions (to find out which are these "needed"
     functions, have a look at which of these functions are being implemented
     in the corresponding "src_linux" folder).
     When implementing these functions for your platform:

       * Pay attention to the provided *.h file where the function prototype is
         declared. These header files include detailed documentation explaining
         what the function (to be implemented by you) does and what values it
         returns.

       * You can use the current linux implementation (found in the
         corresponding "src_linux" folder) as a guide/template for your new
         code.

   3. Add your new "platform" (and "flavour", if needed) to the main "Makefile"
      file.

And that's all! In short, all you have to do is re-implement the body of all of
the "PLATFORM*()" functions so that thay work for your platform and do what is
detailed in the header files.

So that you make an idea of how much work is needed to "port" the code to
another platform, let me through you some numbers:

  * This implementation (as I am writing this) contains ~22.000 lines of source
    (according to "sloccount"),

  * From those, ~19.000 contain platform specific code and ~3.000 contain the
    "linux" platform porting layer.

  * But the "linux" platform includes lots of extra functionality (like the
    option to use "simulated" interfaces). In a more practical approach, a
    porting layer should weight around ~1.500 lines (in fact, I have just
    finished a new porting layer to a custom platform which is exactly that
    long)


## Protocol extensions

There are other protocols, like G.9977 (DPM), designed "on top of" the IEEE1905
standard that add new messages (CMDUs and TLVs).

Likewise, vendors might also want to add their own messages containing new
extra and useful (at least to them!) network information.

Having these new CMDUs/TLVs included in the standard takes a lot of time.
Fortunately, IEEE1905 already provides a mechanism for creating "vendor (or
protocol) specific" extensions (that do not require to modify anything in the
original standard): "Vendor Specific TLVs"
These TLV contain a variable length field, which can be used to include any
type of information. Thus, new non-standard TLV can be embedded inside a
standard Vendor Specific TLV.

Protocol extensions support is implemented on top of this capability.

In fact, *this* IEEE1905 implementation already includes a protocol extension
from the "Broad Band Forum", which other vendors can use as a reference to add
their own.

> NOTE: This BBF protocol extension (their first) defines a new set of TLVs
> used to transmit non-1905 link metrics information.



### Files structure

Protocol extension source files are spread over several directories, following
the same structure than the IEEE1905 standard files.

Only a bunch of new files need to be implemented in order to add a new
extension:

 - **\*_recv.c** and **\*_recv.h**
 - **\*_send.c** and **\*_send.h**
 - **\*_tlvs.c** and **\* tlvs.h**
 - **\*_tlv_forging.c**
 - **\*_tlv_parsing.c**
 - **\*_tlv_test_vectors.c** and **\*_tlv_test_vectors.h**

...where '\*' represents the group/vendor defining this functionality (ex.
'bbf_recv.c')

Each file has a particular purpose:
- **\*_recv.c/h** process the incoming CMDU (and TLVs)
- **\*_send.c/h** extend the outgoing CMDU with your new defined TLVs
- **\*_tlv_forgind.c**, **\*_tlv_parsing.c** and **\*_tlv_test_vectors.c/h** implement
  unit_tests to check your new defined TLVs

Each protocol extension file must be placed inside an 'extension/\*'
subdirectory (once again, '\*' identifies the group/vendor, ex.
'extensions/bbf').

Taking the BBF extension files as an example, this is the path where each file
must be placed:
```
    `-- src/
        |-- al
        |   `-- src_independent
        |       `-- extensions
        |           `-- bbf
        |               |-- bbf_recv.c
        |               |-- bbf_recv.h
        |               |-- bbf_send.c
        |               `-- bbf_send.h
        |
        `-- factory
             |-- interfaces
             |   `-- extensions
             |       `-- bbf
             |           `-- bbf_tlvs.h
             |
             |-- src_independent
             |   `-- extensions
             |       `-- bbf
             |           `-- bbf_tlvs.c
             |
             `-- unit_tests
                 `-- extensions
                     `-- bbf
                         |-- bbf_tlv_forging.c
                         |-- bbf_tlv_parsing.c
                         |-- bbf_tlv_test_vectors.c
                         `-- bbf_tlv_test_vectors.h
```

If BBF ever decides to add new messages, they won't need to create new files.
Instead they will simply have to implement the new funcionality inside the
already existing ones.
However, if you are not the Broad Band Forum and want to add your own custom
extensions, you will need to replicate this file structure as seen next:
```
    `-- src/
        |-- al
        |   `-- src_independent
        |       `-- extensions
        |           |-- bbf
        |           |   |-- bbf_recv.c
        |           |   |-- bbf_recv.h
        |           |   |-- bbf_send.c
        |           |   `-- bbf_send.h
        |           `-- XXX
        |               |-- XXX_recv.c
        |               |-- XXX_recv.h
        |               |-- XXX_send.c
        |               `-- XXX_send.h
        |
        |
        `-- factory
             |-- interfaces
             |   `-- extensions
             |       |-- bbf
             |       |   `-- bbf_tlvs.h
             |       `-- XXX
             |           `-- XXX_tlvs.h
             |
             |
             |-- src_independent
             |   `-- extensions
             |       |-- bbf
             |       |   `-- bbf_tlvs.c
             |       `-- XXX
             |           `-- XXX_tlvs.c
             |
             `-- unit_tests
                 `-- extensions
                     |-- bbf
                     |   |-- bbf_tlv_forging.c
                     |   |-- bbf_tlv_parsing.c
                     |   |-- bbf_tlv_test_vectors.c
                     |   `-- bbf_tlv_test_vectors.h
                     `-- XXX
                         |-- XXX_tlv_forging.c
                         |-- XXX_tlv_parsing.c
                         |-- XXX_tlv_test_vectors.c
                         `-- XXX_tlv_test_vectors.h
```


### Register a group

In order to enable your new protocol extension, you must register your
functionality. This is a two steps process:

  1. Register your new functionality at runtime.

     You must register a set of callbacks at initialization time. These
     callbacks will later be called when apropriated (to process an incoming
     CMDU, to extend an outgoing CMDU, ...)

     This is done in the 'al_extension_register.c' file.

     Take the BBF protocol extension as an example:

     ```
         // BBF extensions
         #include "bbf_send.h"
         #include "bbf_recv.h"

         uint8_t start1905ALProtocolExtension(void)
         {
             // BBF protocol extension
             //
             PLATFORM_PRINTF_DEBUG_DETAIL("Registering BBF protocol extensions...\n");
             if (0 == register1905CmduExtension("BBF", CBKSend1905BBFExtensions, CBKprocess1905BBFExtensions))
             {
                 PLATFORM_PRINTF_DEBUG_ERROR("Could not register BBF protocol extension\n");
                 return 0;
             }

             if (0 == register1905AlmeDumpExtension("BBF",
                                                    CBKObtainBBFExtendedLocalInfo,
                                                    CBKUpdateBBFExtendedInfo,
                                                    CBKDumpBBFExtendedInfo))
             {
                 PLATFORM_PRINTF_DEBUG_ERROR("Could not register BBF datamodel protocol extension\n");
                 return 0;
             }

             // Please, register here your new functionality
             //
             // ...
         }
     ```

  Don't worry about these callbacks yet. We will dive into details in the next
  section.

  2. Enable your new functionality at compilation time.

     Besides registering your new funcionality in the source code, you must
     also add a flag to the build system that lets others decide whether to
     include (or not) your extension by simply setting this flag's value.

     This is done in the main Makefile, located in the 'src' directory. Search
     for the 'Platform independent CONFIGs' comment and add a new flag there to
     enable your functionality (XXX).

     Example:

     ```
       # Platform independent CONFIGs

       #CCFLAGS += -DDO_NOT_ACCEPT_UNAUTHENTICATED_COMMANDS
       CCFLAGS += -DSEND_EMPTY_TLVS
       CCFLAGS += -DFIX_BROKEN_TLVS
       CCFLAGS += -DSPEED_UP_DISCOVERY
         #
         # These are special flags that change the way the implementation behaves.
         # The README file contains more information regarding them.

       CCFLAGS += -D_BUILD_NUMBER_=\"$(shell cat version.txt)\"
         #
         # Version flag to identify the binaries
       CCFLAGS += -DREGISTER_EXTENSION_BBF
       CCFLAGS += -DREGISTER_EXTENSION_XXX    <---------------- (Here)
         #
         # These are special flags to enable Protocol extensions
     ```


### Code overview

Each new protocol extension must:

  1. Define a set of TLVs

     Each protocol extension group is responsible for defining the format of
     its own TLVs. As an example, the BBF group follows the same format defined
     for the standard TLVs (1-byte for TYPE, 2-bytes for LENGTH, and n bytes
     for data). You are, however, free to choose a different format (but why
     would you do that, anyway?)

     TLV definitions and utilities are found in 'XXX_tlvs.c/h'

  2. Implement a set of callbacks

     Protocol extension callbacks are classified in two groups: CMDU extensions
     and Datamodel extensions. Registering the first group is mandatory, while
     the second is optional. Take the BBF implementation (see 'Register a
     group' section) as an example:

     - The first group is registered via 'register1905CmduExtension()'.
       Callbacks belonging to this group are:
       - CBKSend1905XXXExtensions   : Process the incoming CMDU
       - CBKprocess1905XXXExtensions: Extend the outgoing CMDU

     - The second group is registered via 'register1905AlmeDumpExtension()'.
       Callbacks belonging to this group are meant to extended the datamodel
       info when generating a datamodel report (included as response in the
       'dnd' ALME).

       > ALME 'dnd' is not a standard ALME. It is a propietary ALME created to
       > read the whole datamodel. In this sense, Protocol extensions can
       > extend this datamodel report with non-standard info.
       >
       > Registering the second group of callbacks is optional (ie. you might
       > not need to extend the non-standard 'dnd' ALME for your purposes.


Check the previous ('Register a group') section to find a registration
procedure example.



#### TLVs

Proceed like this:

  1. Define an identifier for each TLV

  2. Define a data structure for each TLV

  3. Implement functions to parse, forge and release these TLVs

  4. Implement functions to help unit_tests to test these TLVs

Take the '1905_tlvs.h' file as an example (or the BBF protocol extension
counterpart, 'bbf_tlvs.h' file).

> NOTE: The BBF file does not define any structure for its TLVs because it
> reuses the ones defined in the standard.

The definitions from (1) and (2) must be placed inside 'XXX_tlvs.h'.

The implementations from (3) and (4) must be placed inside 'XXX_tlvs.c'.

Next, define a set of unit_tests to test your TLVs definitions:
  - **XXX_tlv_test_vectors.c**: define a set of test vectors. Each test vector
    comprises a filled structure and a stream
  - **XXX_tlv_forging.c:** define a set of 'forge' tests. Using the functions
    implemented in 'XXX_tlvs.c' converts an input stream in a TLV struture, and
    compare it with the expected result.
  - **XXX_tlv_parsing.c:** define a set of 'parse' tests. Using the functions
    implemented in 'XXX_tlvs.c' converts an input structure in an output
    stream, and compare it with the expected result.

Finally, you need to add your unit_tests to the Makefile found at
'src/factory/unit_tests/Makefile'. Add the next lines:
```
  UNITS += extensions/XXX/XXX_tlv_forging
  UNITS += extensions/XXX/XXX_tlv_parsing
```
To check your TLVs, execute:
```
  $ make clean unit_tests
```


#### Callbacks

##### CMDU extension

There are two CMDU extension callbacks. One is responsible for processing an
incoming CMDU, and the other for extending an outgoing CMDU. They must be
registered via the 'register1905CmduExtension()' API.

  * **Process incoming CMDU**: when a CMDU is received, the main IEEE1905 core
    parses it and builds a data structure, containing a list of TLVs. Each
    registered protocol extension is responsible for running through the whole
    TLV list, parsing the appropriate ones (those Vendor Specific TLVs with a
    specific OUI field) and ignoring the rest.

    This callback must be implemented in the 'XXX_recv.c' file.
    Take 'bbf_recv.c' file as an example.

  * **Extend outgoing CMDU**: when a CMDU is built by the main IEEE1905 core,
    each registered protocol extension has the opportunity to extend the TLV
    list with its own defined TLVs.

    This callback must be implemented in the 'XXX_send.c' file.
    Take 'bbf_send.c' file as a example.



##### Datamodel extension

This IEEE1905 implementation supports a non-standard primitive (called 'dnd')
designed to retrieve the whole datamodel info. It is modeled as a human-
readable text report.

There is also support to extend this report using the non-standard TLVs
(registered by each protocol extension) information.

To make this possible, you must implement three callbacks:

  * **Obtain local info**: get non-standard information belonging to the
    current device, build the appropriate TLVs, and embed each one inside a
    standard Vendor Specific TLV.

    > The stack provides the API to embed your TLV inside a Vendor Specific
    > TLV and to insert this Vendor Specific TLV in the outgoing CMDU
    > ('vendorSpecificTLVEmbedExtension()' and
    > 'vendorSpecificTLVInsertInCDMU()')

  * **Update the datamodel info**: update the datamodel with the received
    protocol extension TLVs. The datamodel simply provides a pointer to a list
    of Vendor Specific TLVs. Each registered Protocol extension is responsible
    for managing this list.

    The received TLVs are related to a particular device, so the AL mac of
    this device is one of the arguments to this callback.

    > Example:
    > BBF defines a set of TLVs to retrieve metrics information from non-1905
    > links. In this particular case, updating the datamodel consists of
    > removing the older TLV metrics and adding the new ones.

    The main stack uses this callback both to update the datamodel local info
    (current device) and the datamodel of neighbor devices. In the first
    case, it obtains the info via the previous callback ("obtain local info"),
    and in the latter case, via the CMDU exchange.

  * **Dump the datamodel**: the main stack calls this callback once for each
    device when generating the report. This function must run through the
    extension TLV's list and add extra information to this report.

These callbacks must be implemented in file 'XXX_send.c'.

Take 'bbf_send.c' as an example, which implements these callbacks:
  - CBKObtainBBFExtendedLocalInfo
  - CBKUpdateBBFExtendedInfo
  - CBKDumpBBFExtendedInfo

> NOTE: why are these three callback needed?
>
> Protocol extensions follow the same procedure as the IEEE1905 stack when
> generating the datamodel report. The 'Dump' callback is executed once for
> each device during this process. That means that the datamodel is expected to
> be updated at this moment.
>
> The 'Update' callback is used to update both the neighbors datamodel section
> when receiving a CMDU or the local datamodel section via the first callback
> (Obtain local info)


## Coding style

In the project fork of the prpl foundation, the coding style has changed. All new code should comply to the following
style guidelines. Existing code will be gradually converted.

  * Use of scalar types:
    - use `bool` for booleans
    - use `size_t` or `ssize_t` for buffer lengths and sizes
    - use `int` or `unsigned` for non-specific types
    - use `uint8_t *` for opaque data (i.e. buffers)
    - only use types from `stdint.h` for specific bit-widths, i.e. pretty much limited to protocol fields
  * Everything must be documented with Doxygen comments.
    - Use Javadoc-style comments, so /** @command */
    - Always use @brief, even if there is only a @brief.
    - Always use full sentences, i.e. start with Capital and terminate with a .
    - Always document all arguments and return value.


# Testing


## Unit tests

Packet forging/parsing unit tests are also included.

In order to execute them, all you have to do is run this:
```
  $ make unit_tests
```

If you ever change something in the packet forging/parsing functions and/or
and a new packet type, please don't forget to make sure the unit tests still
run and/or add new test cases to cover the new functionality.

New test cases are very easy to add:

  1. Add a C structure and its corresponding bit stream representing an ALME,
     CMDU, TLV, ... to the "src/factory/unit_tests/*_test_vectors.[ch]" files.

  2. Add a new entry to the "src/factory/unit_tests/*_forging.c" file.

  3. Add a new entry to the "src/factory/unit_tests/*_parsing.c" file.

  4. Run "make unit_tests" and make sure your new test case appears.



## Static code analysis

In addition to the regular compiler warnings (which should *always* be removed
before committing!), there are additional tools that look at the source code at
a much deeper level.

One of these tools is the "clang" compiler based static analysis tool (called
"scan-build"). It is slow by the report quality is great.

Another one is called "cppccheck", which is much faster and created to generate
a very low number of false positives.

Both of them are run by executing the foolowing commad:
```
  $ make static-analysis
```

Note that for this to work you need to have the following tools installed:

  1. Both ```clang``` and the ```scan-build``` tools (which usually come in the
     same package, typically called "clang")

  2. The ```cppcheck``` tool (usually contained in a package with the same name)

It would be nice if, *before* committing anything to the repo, you make sure
your changes keep the output from these tools empty.

> Note that while these instructions are meant to be run on a regular Linux
> box generating "native" binaries, I guess it would also be possible to make
> it work in a cross-compilation environment. However, the "quality" of the
> generated report would probably be the same... so it is not really worth
> going through all the pain...



## Runtime analysis

Once the binary is built, you can still use some extra tools to detect more
problems.

> As in the previous section, the following  instructions are meant to be used
> in a "native" (i.e. not "cross-compiled") environment. However there is
> certainly way to make them work otherwise.



### Electric fence

While in development, when running a binary, instead of simply executing it, it
is always a good idea to wrap it inside "libefence" and "gdb" like this:
```
  $ LD_PRELOAD=libefence.so.0.0 gdb --args ./<binary_under_test>
```

For example, when running the "al_entity" binary you would do something like
this:
```
  $ LD_PRELOAD=libefence.so.0.0 gdb --args ./al_entity -m 00:00:00:00:00:01 -i eth1,eth0 -vv
```

This will have two combined effects:
  1. Variables are protected with extra padding to detect memory corruptions.
  2. As soon as this happens, the program execution will be halted and you will
     be able to inspect the process state (including the backtrace) in a gdb
     console.

Without ```libefence``` (aka. "electric fence"), memory corruptions can go
undetected for a while, causing damage that might eventually reveal in a
completely unrelated (and hard to debug) way.

> Note that "electric fence" (in particular, its associated *.so library) needs
> to be installed on your system first. The distro package is called
> "electricfence", "electric-fence" or something like that.



### Valgrind

The previous "electric fence" tool is quite simple and straight forward and you
should probably use it as often as possible.

This other tool, on the other hand, requires some more work and should probably
only be added at some later point of your workflow.
*However*, please *do use it* (specially to detect memory leaks).

To you it, execute your binary like this:
```
    valgrind --log-file=val_%p.txt --time-stamp=yes --gen-suppressions=all --tool=memcheck --track-origins=yes --leak-check=full <binary_under_test>
```

For example, when running the "al_entity" binary you would do something like
this:
```
    valgrind --log-file=val_%p.txt --time-stamp=yes --gen-suppressions=all --tool=memcheck --track-origins=yes --leak-check=full ./al_entity -m 00:00:00:00:00:01 -i eth1,eth0 -vv
```

...then let the binary run for a while. Exercise it. In our case, make it talk
to other 1905 nodes so that as many execution paths as possible are visited.

After a while close your binary ("CTRL+c" in our case) and look at the generated
report file (names "val_xxx.txt").

> Note that this report might contain several entries regarding memory leaks
> detected when "pthread_create()" is executed. This is due to the fact of
> threads not being "join()'ed" at the end of their life. You can ignore these
> but not others!
> Try to keep these reports as empty as possible.


## Black box tests

I *manually* run the following tests from time to time. In the future I would
like to automate them.



### Push button configuration



#### Scenario 1

  A) Configured AP WIFI registrar (1 eth + 1 wlan AP)

  B) Unregistered STA WIFI (1 wlan)

```
  Time     Event                    [A]                      [B]
  ---------------------------------------------------------------------------
  0        Button is pressed        - Start PB on eth
           on [A]                   - Start PB on wlan

  1        Button is pressed                                 - Start PB on wlan
           on [B]

  2        [B] receives PB          - Send notification      - <nothing> (*)
           configuration data         on eth
           from [A]

  (*) Notifications are only sent over *previously* authenticated interfaces
```



##### Scenario 2

  A) Configured WIFI AP registrar (1 eth + 1 wlan AP)

  B) Already registered STA WIFI (1 wlan)

```
  Time     Event                    [A]                      [B]
  ---------------------------------------------------------------------------
  0        Button is pressed        - Start PB on eth
           on [A]                   - Start PB on wlan

  1        Button is pressed                                 <nothing> (*)
           on [B]

  2        PB timeout

  (*) Already registered STAs do not start a new PB process
```



##### Scenario 3

  A) Configured WIFI AP registrar (1 eth + 1 wlan AP)

  B) Unconfigured WIFI AP (1 eth + 1 wlan AP)

```
  Time     Event                    [A]                      [B]
  ---------------------------------------------------------------------------
  0        Button is pressed        - Start PB on eth
           on [A]                   - Start PB on wlan

  1        Button is pressed                                 - Start PB on eth
           on [B]                                            - Start PB on wlan (*1)

  2        eth interfaces (in [A]   <nothing> (*3)           - Send AP-autoconf
           and [B] become                                      search
           authenticated) (*2)

  (*1) Because both [A] and [B] are access points, the PB mechanism will do
       nothing.

  (*2) This actually happens as soon as the buttons of [A] and [B] are pressed,
       but to keep the timeline clean, we will consider this as a separate
       event.

  (*3) Only devices with an unconfigured AP interface send "AP-autoconf" search
       messages
```



### Access Point security settings cloning



#### TC001

**Setup:**
  1. PC with two Ethernet interfaces: eth0, eth1
  2. Create file "interface.eth0.sim" with these contents:
     ```

       mac_address                                   = 00:16:03:01:85:1f
       manufacturer_name                             = Marvell
       model_name                                    = WIFI PHY x200
       model_number                                  = 12345
       serial_number                                 = 100000000000
       device_name                                   = Marvell WIFI phy x200
       uuid                                          = 100000000000
       interface_type                                = INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ
       ieee80211.bssid                               = 00:16:03:01:85:1f
       ieee80211.ssid                                = My WIFI network
       ieee80211.role                                = IEEE80211_ROLE_AP
       ieee80211.ap_channel_band                     = 10
       ieee80211.ap_channel_center_frequency_index_1 = 20
       ieee80211.ap_channel_center_frequency_index_2 = 30
       ieee80211.authentication_mode                 = IEEE80211_AUTH_MODE_WPAPSK
       ieee80211.encryption_mode                     = IEEE80211_ENCRYPTION_MODE_AES
       ieee80211.network_key                         = Test network
       is_secured                                    = 1
       push_button_on_going                          = 0
       push_button_new_mac_address                   = 00:00:00:00:00:00
       power_state                                   = INTERFACE_POWER_STATE_ON
    ```
  3. Create file "interface.eth1.sim" with these contents:
     ```

       mac_address                                   = 38:ea:a7:11:84:dc
       manufacturer_name                             = Marvell
       model_name                                    = ETH PHY x200
       model_number                                  = 99999
       serial_number                                 = 100000000001
       device_name                                   = Marvell eth phy x200
       uuid                                          = 0000000000810001
       interface_type                                = INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET
       is_secured                                    = 1
       push_button_on_going                          = 2
       push_button_new_mac_address                   = 00:00:00:00:00:00
       power_state                                   = INTERFACE_POWER_STATE_ON
     ```

**Execution:**
  1. Run this command:
     ```

       $ al_entity -m aa:aa:aa:aa:aa:00 -i eth1:simulated:interface.eth1.sim,eth0:simulated:interface.eth0.sim -v
     ```
  2. Wait a few seconds, open a new terminal and execute this:
     ```

       $ touch /tmp/virtual_push_button
     ```
  3. Finish the "al_entity" process with CTRL+c

**Expected output:**
```
  INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)
  INFO    : --> LLDP BRIDGE DISCOVERY (eth1)
  INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth0)
  INFO    : --> LLDP BRIDGE DISCOVERY (eth0)

  INFO    : Starting push button configuration process on interface eth0
  INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth1)
  INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth0)
```

**Explanation:**

This node is running an already configured access point (on eth0) and thus the
"discovery" messages are sent through both interfaces (eth0 and eth1).
When the virtual button is pressed, the "push button configuration process" is
started only in eth0 (because eth1, being Ethernet, does not include support
for this), however the notification CMDU is sent through both interfaces.



#### TC002

**Setup:**
  1. PC with two Ethernet interfaces: eth0, eth1
  2. Create file "interface.eth0.sim" with these contents:
     ```

       mac_address                                   = 00:16:03:02:85:20
       manufacturer_name                             = Marvell
       model_name                                    = WIFI PHY x200
       model_number                                  = 12345
       serial_number                                 = 200000000000
       device_name                                   = Marvell WIFI phy x200
       uuid                                          = 200000000000
       interface_type                                = INTERFACE_TYPE_IEEE_802_11G_2_4_GHZ
       ieee80211.bssid                               = 00:00:00:00:00:00
       ieee80211.ssid                                =
       ieee80211.role                                = IEEE80211_ROLE_AP
       ieee80211.ap_channel_band                     = 00
       ieee80211.ap_channel_center_frequency_index_1 = 00
       ieee80211.ap_channel_center_frequency_index_2 = 00
       ieee80211.authentication_mode                 = IEEE80211_AUTH_MODE_OPEN | IEEE80211_AUTH_MODE_WPAPSK
       ieee80211.encryption_mode                     = IEEE80211_ENCRYPTION_MODE_AES | IEEE80211_ENCRYPTION_MODE_TKIP
       ieee80211.network_key                         =
       is_secured                                    = 0
       push_button_on_going                          = 0
       push_button_new_mac_address                   = 00:00:00:00:00:00
       power_state                                   = INTERFACE_POWER_STATE_ON
     ```
  3. Create file "interface.eth1.sim" with these contents:
     ```

       mac_address                                   = 38:ea:a7:11:84:dd
       manufacturer_name                             = Marvell
       model_name                                    = ETH PHY x200
       model_number                                  = 99999
       serial_number                                 = 0000000000810001
       device_name                                   = Marvell eth phy x200
       uuid                                          = 200000000001
       interface_type                                = INTERFACE_TYPE_IEEE_802_3U_FAST_ETHERNET
       is_secured                                    = 1
       push_button_on_going                          = 2
       push_button_new_mac_address                   = 00:00:00:00:00:00
       power_state                                   = INTERFACE_POWER_STATE_ON
     ```

**Execution:**
  1. Run this command:
     ```

       $ al_entity -m aa:aa:aa:aa:aa:00 -i eth1:simulated:interface.eth1.sim,eth0:simulated:interface.eth0.sim -v
     ```
  2. Wait a few seconds, open a new terminal and execute this:
     ```

       $ touch /tmp/virtual_push_button
     ```
  3. Finish the "al_entity" process with CTRL+c

**Expected output:**
```
 INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)
 INFO    : --> LLDP BRIDGE DISCOVERY (eth1)

 INFO    : Starting push button configuration process on interface eth0
 INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (eth1)
 INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth1)
```

**Explanation:**

This is the same as TC001, but this time we have an *unconfigured* access point
(on eth0), thus the initial "discovery" messages are only sent through eth1.
When the virtual button is pressed, the same things as in TC001 happens except
for one new item:  because this node contains an unconfigured AP interface
(eth0), a "search" CMDU is also sent through the authenticated interface
(eth1).



#### TC003

**Setup:**
  1. PC#1 = PC from TC001 (with its configuration files)
  2. PC#2 = PC from TC002 (with its configuration files)
  3. PC#1/eth0 <--- Ethernet ---> PC#2/eth0
  4. PC#1/eth1 <--- Ethernet ---> PC#2/eth1
     ```

       NOTE: It doesn't really need to be Ethernet. ANYTHING will do as long
       as there is connectivity, because the actual parameters for each
       interface will be retrieved from the simulation file.
     ```

**Execution:**
  1. On PC#1, run this command:
     ```

       $ al_entity -m aa:aa:aa:aa:aa:00 -i eth1:simulated:interface.eth1.sim,eth0:simulated:interface.eth0.sim -v
     ```
  2. Wait a few seconds. On PC#2, run this command:
     ```

       $ al_entity -m aa:aa:aa:aa:aa:11 -i eth1:simulated:interface.eth1.sim,eth0:simulated:interface.eth0.sim -v
     ```
  3. Wait a few seconds. On PC#2, open a new terminal and execute this:
     ```

       $ touch /tmp/virtual_push_button
     ```
  4. Wait a few seconds.
  5. Finish the "al_entity" process on PC#1 and PC#2 with CTRL+c

**Expected output:**
```
 On PC#1:                                                                                   On PC#2:
 ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)
   INFO    : --> LLDP BRIDGE DISCOVERY (eth1)
   INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth0)
   INFO    : --> LLDP BRIDGE DISCOVERY (eth0)

   INFO    : <-- CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)                                          INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)
   INFO    : --> CMDU_TYPE_TOPOLOGY_QUERY (eth1)                                              INFO    : --> LLDP BRIDGE DISCOVERY (eth1)
   INFO    : <-- LLDP BRIDGE DISCOVERY (eth1)                                                 INFO    : <-- CMDU_TYPE_TOPOLOGY_QUERY (eth1)
   INFO    : <-- CMDU_TYPE_TOPOLOGY_RESPONSE (eth1)                                           INFO    : --> CMDU_TYPE_TOPOLOGY_RESPONSE (eth1)
   INFO    : --> CMDU_TYPE_LINK_METRIC_QUERY (eth1)                                           INFO    : <-- CMDU_TYPE_LINK_METRIC_QUERY (eth1)
   INFO    : --> CMDU_TYPE_HIGHER_LAYER_QUERY (eth1)                                          INFO    : --> CMDU_TYPE_LINK_METRIC_RESPONSE (eth1)
   INFO    : <-- CMDU_TYPE_LINK_METRIC_RESPONSE (eth1)                                        INFO    : <-- CMDU_TYPE_HIGHER_LAYER_QUERY (eth1)
   INFO    : <-- CMDU_TYPE_LINK_METRIC_RESPONSE (eth1)                                        INFO    : --> CMDU_TYPE_HIGHER_LAYER_RESPONSE (eth1)

   INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (eth1)                                 INFO    : Starting push button configuration process on interface eth0
   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (forwarding from eth1 to eth0)         INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (eth1)
   INFO    : <-- CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth1)                              INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth1)
   INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (forwarding from eth1 to eth0)
```

**Explanation:**

Here we are connecting the PCs from TC001 and TC002 together.

When the virtual button is pressed on PC#2 (which is the one with an
unconfigured AP interface), the same things as in TC002 happen, but this time
there someone listening at the other end of eth1.

When PC#1 receives the CMDU messages ("push button notification" and "AP
autoconfiguration search"), this is what happens:

    1. Because these CMDUs are of type "relayed multicast", they are forwarded
       to eth1.

    2. The "push button configuration process" is *not* started in any of the
       interfaces of PC#1 because:
         a) "eth0" is not the network registrar
         b) "eth1" (Ethernet) does not support it

**Notes:**

In the actual output, in addition to the "INFO" messages detailed above, you
will probably also see "WARNING" messages.
Two of them can be ignored (and only these two! if any other "WARNING" message
appears it should be considered an error):

  * `WARNING : Unknown destination AL MAC. Using the 'src' MAC from the
    METRICS QUERY (aa:aa:aa:aa:aa:00)`
    This is, in fact, a problem with the 1905 standard itself (see the
    "problems ins the IEEE 1905 standard" section of this same README.md file)

  * `WARNING : This interface (eth0) is not secured. No packets should be
    received. Ignoring...`
    This is an artifact of the simulation environment: when "relayed multicast"
    CMDUs are forwarded (in PC#1) from eth0 to eth1, they should never be
    received by PC#2 (because PC#2/eth0 is not yet authenticated), but in a
    simulation environment they do.



#### TC004

**Setup:**
  1. The *same* as TC003

**Execution:**
  1. The *same* as TC003, but when running "al_entity" on PC#1, add "-r eth0":
     ```

       $ al_entity -m aa:aa:aa:aa:aa:00 -i eth1:simulated:interface.eth1.sim,eth0:simulated:interface.eth0.sim -r eth0 -v
     ```

**Expected output:**
```
 On PC#1:                                                                                   On PC#2:
 ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)
   INFO    : --> LLDP BRIDGE DISCOVERY (eth1)
   INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth0)
   INFO    : --> LLDP BRIDGE DISCOVERY (eth0)

   INFO    : <-- CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)                                        INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)
   INFO    : --> CMDU_TYPE_TOPOLOGY_QUERY (eth1)                                            INFO    : --> LLDP BRIDGE DISCOVERY (eth1)
   INFO    : <-- LLDP BRIDGE DISCOVERY (eth1)                                               INFO    : <-- CMDU_TYPE_TOPOLOGY_QUERY (eth1)
   INFO    : <-- CMDU_TYPE_TOPOLOGY_RESPONSE (eth1)                                         INFO    : --> CMDU_TYPE_TOPOLOGY_RESPONSE (eth1)
   INFO    : --> CMDU_TYPE_LINK_METRIC_QUERY (eth1)                                         INFO    : <-- CMDU_TYPE_LINK_METRIC_QUERY (eth1)
   INFO    : --> CMDU_TYPE_HIGHER_LAYER_QUERY (eth1)                                        INFO    : --> CMDU_TYPE_LINK_METRIC_RESPONSE (eth1)
   INFO    : <-- CMDU_TYPE_LINK_METRIC_RESPONSE (eth1)                                      INFO    : <-- CMDU_TYPE_HIGHER_LAYER_QUERY (eth1)
   INFO    : <-- CMDU_TYPE_HIGHER_LAYER_RESPONSE (eth1)                                     INFO    : --> CMDU_TYPE_HIGHER_LAYER_RESPONSE (eth1)



   INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (eth1)                               INFO    : Starting push button configuration process on interface eth0
   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE (eth1)                             INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (eth1)
   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (forwarding from eth1 to eth0)       INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth1)
   INFO    : <-- CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth1)                            INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE (eth1)
   INFO    : Starting push button configuration process on interface eth0                   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)
   INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (forwarding from eth1 to eth0)    INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)
   INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)                                  INFO    : Applying WSC configuration (eth0):
   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)                                  INFO    :   - SSID            : My WIFI network
                                                                                            INFO    :   - BSSID           : 00:16:08:01:8a:f1
                                                                                            INFO    :   - AUTH_TYPE       : 0x0002
                                                                                            INFO    :   - ENCRYPTION_TYPE : 0x0008
                                                                                            INFO    :   - NETWORK_KEY     : Test network
```

**Explanation:**

This time PC#1 *is* the network registrar, thus when the CMDUs arrive:

  1. Because these CMDUs are of type "relayed multicast", they are forwarded to
     eth1 (same as in TC003)

  2. The "push button configuration process" is now started on eth0 because it
     is the registrar.

  3. A response is sent back (this is how PC#1 tells PC#2 that he is the network
     registrar)

When PC#2 receives the response, it then forges and sends a WSC message
(containing a "M1" payload), and then PC#1 receives it and responds with a
final WSC message (containing a "M2" payload).

After all of this, PC#2 should now contained a configured AP using the same
parameters as those provided by the registrar.

**Notes:**

Ignore the same "WARNING" messages as in TC003



#### TC005

**Setup:**
  1. The *same* as TC004, but this time interfaces "eth1" on PC#2 and PC#3
     will be "dummy G.hn" interfaces connected over a G.hn link.
     ```

       PC#1/eth0 <----- Ethernet -----> PC#2/eth0
       PC#1/eth1 <-- X ~~~G.hn~~~ X --> PC#2/eth1

         NOTE: The "X" marks denote "dummy" G.hn interfaces whose MAC addresses
         are "<X1>" and "<X2>" and their LCMP password is "<P1>" and "<P2>"
     ```
  2. Only files named "interface.eth0.sim" will be needed (you can delete -or
     not- the other ones).
  3. Initially, G.hn modem <X1> must be "secured". You can achieve this by
     manually starting a pairing process by itself (e.g. no one responds) and
     waiting:
       - Set parameter "PAIRING.GENERAL.PROCESS_START" to "1":
         ```

           > configlayer -i eth1 -m <X1> -o SET -p PAIRING.GENERAL.PROCESS_START=1 -w <P1>
         ```
       - Keep reading that same parameter until its value changes back to "0":
         ```

           > configlayer -i eth1 -m <X1> -o GET -p PAIRING.GENERAL.PROCESS_START -w <P1>
         ```
  4. Initially, G.hn modem <X2> must be "unpaired". You can achieve this by
     manually setting parameter "PAIRING.GENERAL.LEAVE_SECURE_DOMAIN" to "1":
     ```

        > configlayer -i eth1 -m <X2> -o SET -p PAIRING.GENERAL.LEAVE_SECURE_DOMAIN=1 -w <P2>
     ```

**Execution:**
  1. On PC#1, run this command:
     ```

       $ al_entity -m aa:aa:aa:aa:aa:00 -i eth1:ghnspirit:<X1>:<P1>,eth0:simulated:interface.eth0.sim -r eth0 -v
     ```
     Example:
     ```
       $ al_entity -m aa:aa:aa:aa:aa:00 -i eth1:ghnspirit:00139D00111F:bluemoon,eth0:simulated:interface.eth0.sim -r eth0 -v
     ```
  2. Wait a few seconds. On PC#2, run this command:
     ```
       $ al_entity -m aa:aa:aa:aa:aa:11 -i eth1:ghnspirit:<X2>:<P2>,eth0:simulated:interface.eth0.sim -v
     ```
     Example:
     ```

       $ al_entity -m aa:aa:aa:aa:aa:11 -i eth1:ghnspirit:00139D001103:palemoon,eth0:simulated:interface.eth0.sim -v
     ```
  4. Wait a few seconds. On PC#1, open a new terminal and execute this:
     ```

       $ touch /tmp/virtual_push_button
     ```
  5. Wait a few seconds. On PC#2, open a new terminal and execute this:
     ```

       $ touch /tmp/virtual_push_button
     ```
  6. Wait a few seconds (or minutes) until the pairing process ends.
  7. Finish the "al_entity" process on PC#1 and PC#2 with CTRL+c

**Expected output:**
```
 On PC#1:                                                                                   On PC#2:
 ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth1)
   INFO    : --> LLDP BRIDGE DISCOVERY (eth1)
   INFO    : --> CMDU_TYPE_TOPOLOGY_DISCOVERY (eth0)
   INFO    : --> LLDP BRIDGE DISCOVERY (eth0)

   INFO    : Starting push button configuration process on interface eth1
   INFO    : Starting push button configuration process on interface eth0
   INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth1)
   INFO    : --> CMDU_TYPE_PUSH_BUTTON_EVENT_NOTIFICATION (eth0)

                                                                                            INFO    : Starting push button configuration process on interface eth1
                                                                                            INFO    : Starting push button configuration process on interface eth0

   INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (eth1)                               INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (eth1)
   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE (eth1)                             INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_RESPONSE (eth1)
   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_SEARCH (forwarding from eth1 to eth0)       INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)
   INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)                                  INFO    : <-- CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)
   INFO    : --> CMDU_TYPE_AP_AUTOCONFIGURATION_WSC (eth1)                                  INFO    : Applying WSC configuration:
                                                                                            INFO    :   - SSID            : My WIFI network
                                                                                            INFO    :   - BSSID           : 00:16:08:01:8a:f1
                                                                                            INFO    :   - AUTH_TYPE       : 0x0002
                                                                                            INFO    :   - ENCRYPTION_TYPE : 0x0008
                                                                                            INFO    :   - NETWORK_KEY     : Test network
```

**Explanation:**

As in TC004, PC#1 eth0 is the registrar, whose credentials are going to be
cloned by PC#2 eth0.

In order for this to happen, the G.hn channel needs to be secured first. Then
PC#2 sends an "AP autoconfiguration search packet" and credentials are cloned.

Differences to notice versus TC004:

    1. All the initial topology and metric queries are not being sent because
       initially there are no secured links.

    2. The same thing happens to the "push button event notification" message
       from PC#2: it is never generated because at that point (when the button
       is pressed) the interface is not yet secured (while on TC004, Ethernet
       interfaces are always considered to be secured).

**Notes:**

Ignore the same "WARNING" messages as in TC003



# TODO

These are items that we should implement/fix, listed in order of importance:

 - Support for "forwarding" commands on Linux (but first, we need to define
   what are the "use case" scenarios for taking advantage of forwarding)

 - Network registrar detection (1A: 10.1.4 & Annex E)

 - U-key derivation (9.2.1)

 - TR069 datamodel based on BBF TR-181

 - Security analysis! (Check for buffer overflows, malformed packets handling,
   etc...)

 - Add intelligence to the HLE (out of spec, but needed anyway, at least as
   a reference implementation with a very simple intelligence that can later
   be improved)

 - Automatic test virtualized scenarios (I'm considering "cloonix")

There are also several places in the source code marked with the "TODO" tag.
They are a good place to start contributing! :)



# Appendix A: OpenWRT tool chain and flash image generation

In this section we are going to explain how to obtain and use the OpenWRT tools
to generate a final flash image containing a pthreads-enabled Linux kernel.

  <TODO>



# Appendix B: Installing "minGW"

  <TODO>

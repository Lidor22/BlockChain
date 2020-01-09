# BlockChain -Introduction :
As a part of our Embedded Linux course , we created our own version of the BlockChain.
Our system starts from a general genesis block and uses 4 miners in order to mine more blocks to the blocks list.
The project was developed in Visual Code using C programming language on Ubunutu 18.04 version.

During this Project we learned elements such as :

* Developing project in Linux environment.
* Working with multi threading(Locks,Condition Variables).
* Creating and using a MakeFile.
* Using GDB to debugg the code in Linux.
* Getting familliar with BlockChain aspects.

# Our Project:

The main goal of the system is to demonstrate how the real BlockChain system works.
It starts from empty blocks list and make the miners "mine" new blocks .
Each block contains several fields such as: height(Incrementeal ID of the block in the chain), timestamp(Time of the mine in seconds since epoch), hash(Current block hash value), prev_hash(Hash value of the previous block), difficulty(Amount of preceding zeros in the hash), nonce(Incremental integer to change the hash value), relayed_by(Miner ID).

Basically the main function creates 6 threades :
* First thread is the Server thread which is responsible on adding the new blocks to the chain and to validate  the hash value that was calculate by the miner who mined the block.
* 4 threads are used as Miners - they calculate certain hash value based on the block values such as height,prev_hash,minder id etc ... Once the hash value that was calculated matches the difficulty required - the miner needs to inform the server in order to add the new mined block to the BlockChain.
* Last thread is a false thread that sends wrong hash value to the server and inform him about a new block that was mined. The purpose of this thread is to test the ability of the server to validate the authenticity of the hash value.


In order to activate the system - please use the MakeFile and run the bitcoin.out file.







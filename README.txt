To the assignment tester:
We implemented our Discover function in such way that it can avoid circuits.
Our way was to save the predecessor of the discover request (the node that initialized the request) 
in a map so if we already got a request we won't flood from our node. 
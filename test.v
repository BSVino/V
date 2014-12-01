test := ()
{
	return test2() + 1;
}

test2 := ()
{
	return 41;
}

main := ()
{
	return test();
}

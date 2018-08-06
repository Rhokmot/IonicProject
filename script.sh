for i in {1..100}
do
	mpirun –n 4 ./mpi22 >> experiment_1_4.csv
done

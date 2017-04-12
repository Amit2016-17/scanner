from scannerpy import Database, TableJob

################################################################################
# This tutorial shows how to combine multiple operators into a computation     #
# graph and wire inputs/outputs.                                               #
################################################################################

with Database() as db:

    # Scanner can take a directed acyclic graph (DAG) of operators and pass data
    # between them. Each graph has an Input node at the beginning that represents
    # the data from the input table.

    tables, _ = db.ingest_videos([('example', '/tmp/example.mp4')], force=True)

    jobs = []
    for t in tables:
        frame, frame_info = t.as_op()

        blurred_frame, _ = db.ops.Blur(
            frame = frame,
            frame_info = frame_info,
            kernel_size = 3,
            sigma = 0.5)

        histogram = db.ops.Histogram(
            frame = blurred_frame,
            frame_info = frame_info)

        jobs.append(TableJob(
            rows=t.rows().all(),
            columns=[histogram],
            name='output_table_name'))

    db.run(jobs, force=True)

    # Note: if you don't explicitly include an Input or Output node in your op graph
    # they will be automatically added for you. This is how the previous examples
    # have worked.

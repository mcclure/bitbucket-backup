# "Sensor" example from tutorial
# Expect:
# Set key testValue to 15.
# Read key testValue
# 15.

sensor = [
    secrets = []              # This is the real object

    # Catch attempts to read a field...
    parent ^key = (
        print "Read key " key ln               # Gossip
        this.secrets key      # Then forward to secrets
    )

    # Catch attempts to write a field...
    let ^key ^value = (
        print "Set key "  key " to " value ln
        this.secrets key = value   # Forward to secrets
    )
]

sensor.testValue = 5 + 10

println: sensor.testValue
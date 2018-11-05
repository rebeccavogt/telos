# producerjson

Producer JSON smart contract for Telos.

# Usage

### View the table

```
teclos get table producerjson producerjson producerjson
```

### Add to the table

```
teclos push action producerjson set '{"owner":"your_account", "json": "your_json"}' -p your_account@active
```

**Example**:
```
teclos push action producerjson set '{"owner":"telosmiamibp", "json": "'`printf %q $(cat bp.json | tr -d "\r")`'"}' -p telosmiamibp@active
```

### Remove from the table

```
teclos push action producerjson del '{"owner":"your_account"}' -p your_account@active
```


---

# How to build
```
./build.sh
```
or
```
eosiocpp -g producerjson.abi producerjson.cpp && eosiocpp -o producerjson.wast producerjson.cpp
```

# How to deploy

```
teclos set contract producerjson producerjson -p producerjson@active
```

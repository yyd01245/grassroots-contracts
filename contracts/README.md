# Grassroots Contract API

Grassroots is a crowdfunding development platform for EOSIO software. It allows account holders to vote native system tokens to fund projects, applications, campaigns, or even research and development.

## Register Your Account

To begin using the Grassroots platform users simply call the `grassroots::registeracct` action.

* registeracct(name account_name)`

    `account_name` is the name of the account to be registered in Grassroots. Currently users must enter their own account names into this field.

Alternatively, users can create an account by simply making a regular `eosio.token::transfer` to the `@gograssroots` account with a memo of "register account" (without the quotes). This will create a balance entry with RAM paid for by Grassroots, but will charge an account creation fee of `0.1 TLOS` before placing the remainder of the transfer into the newly created account. This is a really easy way to register for a Grassroots account if using a wallet that is limited to transfer functionality.

After creating a Grassroots account, all future `eosio.token::transfers` to `@gograssroots` will be caught by the contract and placed in the sender's Grassroots account free of charge.

A user's Grassroots account is their operating balance for all actions on the platform. This means all contributions, donations, and fees are pulled from this account. If at any time a user experiences an `insufficient balance` error, they can simply transfer more `TLOS` to `@gograssroots` to debit their account.

## Creating A Project

Project creation in Grassroots is easy, just follow this track:

### Project Setup

Creating a new project in Grassroots is done by calling the `grassroots::newproject` action.

When setting string variables, please use markdown format. Grassroots React components are configured to parse markdown.

* `newproject(name project_name, name category, name creator, string title, string description, asset requested)`

    `project_name` is the name of the new project. The project name must conform to the `eosio::name` encoding (a-z1-5, max 12 characters).

    `category` is the category for the new project. Select the category from the available list below.

    `creator` is the name of the creator, and must be the signer of this action.

    `title` is the title of the project.

    `description` is a brief description of the project.

    `requested` is the amount of `TLOS` requested to fund the project.

Available categories: `apps, audio, environment, expansion, games, marketing, publishing, research, technology, video`

### Adding Preorders

**`In Development...`**

### Editing Project Details

To edit a project's details, simply call the `grassroots::updateproj` action. If any edits need to be made they must be done before opening the project for funding. Once a project has begun funding it can no longer be edited by the project creator until the campaign is over.

* `updateproj`(name project_name, name creator, string new_title, string new_desc, string new_link, asset new_requested)

    `project_name` is the name of the project to edit.

    `creator` is the name of the project creator. Only this account is authorized to edit the project.

    `new_title` is the new title.

    `new_desc` is the new description.

    `new_link` is the new info link.

    `new_requested` is the new requested amount for the project.

To leave a field unchanged, type "none" in its field. **In Development...**

### Begin Funding Campaign

After project setup is complete, the final step is to open the project for funding by calling the `grassroots::openfunding` action.

* `openfunding(name project_name, name creator, uint8_t length_in_days)`

    `project_name` is the to be opened for funding.

    `creator` is the project creator. Only this account is authorized to open the project for funding.

    `length_in_days` is the number of days funding will be open, from the moment the `openfunding()` action is called.

Note that there is a flat `25 TLOS` fee for opening funding on a project.

### Wait for Contributions/Donations

`Still Writing...`

### Deleting A Project

`Still Writing...`

### Cancelling A Project

`Still Writing...`



## Contributing to Projects

Below is a guide to interacting with the Grassroots platform and contributing to projects as a general user.

### Discover Projects

Browse a huge project catalogue with a wide variety of categories.

All: `cleos get table gograssroots gograssroots projects`

By Category: `cleos get table gograssroots gograssroots projects --lower category --key-type i64 --index 2`

* `games` : 

* `apps` : 

* `research` : 

* `technology` : 

* `environment` : 

* `audio` : 

* `video` : 

* `publishing` : 

### Make a Donation

To simply donate to a project without making a purchase, call the `grassroots::donate` action.

* `donate(name project_name, name donor, asset amount, string memo)`

    `project_name` is the name of the project receiving the donation.

    `donor` is the account name making the donation.

    `amount` is the quantity of system tokens to donate.

    `memo` is a brief memo for the project creator.

### Withdraw Funds

To withdraw funds from a Grassroots balance back to a regular `eosio.token` balance, simply call the `grassroots::withdraw` action. Users can withdraw an amount up to their Grassroots account balance.

* `withdraw(name account_name, asset amount)`

    `account_name` is the name of the Grassroots account to withdraw from. Only the owner of the account can withdraw from it.

    `amount` is the quantity of system tokens to withdraw from the Grasroots account.
